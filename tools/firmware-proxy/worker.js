// Cloudflare Worker - CORS proxy for Vail GitHub release assets
// Deploy: npx wrangler deploy
//
// Routes (both supported):
//   /{tag}/{file}.bin                 -> Vail-CW/vail-summit  (legacy, unchanged)
//   /{repo}/{tag}/{file}.(bin|uf2|hex) -> Vail-CW/{repo}      (e.g. vail-adapter)
//
// Only Vail-CW repos and firmware extensions are allowed.
//
// GitHub throttles Cloudflare Workers egress IPs, so fetches to github.com
// intermittently fail with 520 and stay failed for the whole invocation.
// The signed release-assets.githubusercontent.com URLs are reliable, though,
// so we resolve the redirect ourselves and fall back to the authenticated
// GitHub API (GITHUB_TOKEN secret) when the github.com hop is down.

const ALLOWED_REPOS = ['vail-summit', 'vail-adapter'];
const CORS = { 'Access-Control-Allow-Origin': '*' };

// Per-isolate cache of release JSON so API fallback lookups don't burn
// rate limit on every download. Signed URLs expire, so cache the asset
// list, not the resolved location.
const releaseCache = new Map();
const RELEASE_CACHE_MS = 10 * 60 * 1000;

// Resolve the signed asset URL via the plain github.com redirect.
// Returns {location} on success, {notFound: true} for a missing asset,
// or null when github.com is unreachable from this egress IP.
async function resolveViaGithubCom(repo, tag, filename) {
  const url = `https://github.com/Vail-CW/${repo}/releases/download/${tag}/${filename}`;
  for (let attempt = 1; attempt <= 2; attempt++) {
    let resp;
    try {
      resp = await fetch(url, { redirect: 'manual' });
    } catch (err) {
      continue;
    }
    if (resp.status === 301 || resp.status === 302) {
      const loc = resp.headers.get('Location');
      if (loc) return { location: loc };
    }
    if (resp.status === 404) return { notFound: true };
  }
  return null;
}

// Fallback: resolve via the GitHub API with the worker's token. The API
// frontend accepts connections from Workers egress; only the per-IP
// unauthenticated rate limit is a problem, which the token avoids.
async function resolveViaApi(repo, tag, filename, token) {
  const auth = {
    'User-Agent': 'vail-firmware-proxy',
    'Authorization': `Bearer ${token}`,
  };
  const key = `${repo}@${tag}`;
  let cached = releaseCache.get(key);
  if (!cached || Date.now() - cached.at > RELEASE_CACHE_MS) {
    const resp = await fetch(`https://api.github.com/repos/Vail-CW/${repo}/releases/tags/${tag}`, {
      headers: { ...auth, 'Accept': 'application/vnd.github+json' },
    });
    if (resp.status === 404) return { notFound: true };
    if (!resp.ok) return null;
    const json = await resp.json();
    cached = { assets: json.assets || [], at: Date.now() };
    releaseCache.set(key, cached);
  }
  const asset = cached.assets.find(a => a.name === filename);
  if (!asset) return { notFound: true };
  const dl = await fetch(asset.url, {
    redirect: 'manual',
    headers: { ...auth, 'Accept': 'application/octet-stream' },
  });
  if (dl.status === 301 || dl.status === 302) {
    const loc = dl.headers.get('Location');
    if (loc) return { location: loc };
  }
  if (dl.ok) return { response: dl };
  return null;
}

export default {
  async fetch(request, env) {
    // Handle CORS preflight
    if (request.method === 'OPTIONS') {
      return new Response(null, {
        headers: {
          ...CORS,
          'Access-Control-Allow-Methods': 'GET',
          'Access-Control-Allow-Headers': '*',
        }
      });
    }

    // Only allow GET
    if (request.method !== 'GET') {
      return new Response('Method not allowed', { status: 405 });
    }

    const url = new URL(request.url);
    let repo, tag, filename;

    // New form: /{repo}/{tag}/{file}.(bin|uf2|hex)
    // Tag allows SemVer pre-release suffixes too, e.g. v5.0.1-beta.1
    let match = url.pathname.match(/^\/([\w-]+)\/(v[\w.-]+)\/([\w.-]+\.(?:bin|uf2|hex))$/);
    if (match) {
      [, repo, tag, filename] = match;
    } else {
      // Legacy Summit form: /{tag}/{file}.bin
      match = url.pathname.match(/^\/(v[\d.]+)\/([\w.-]+\.bin)$/);
      if (match) {
        repo = 'vail-summit';
        [, tag, filename] = match;
      }
    }

    if (!match || !ALLOWED_REPOS.includes(repo)) {
      return new Response('Not found. Use /{repo}/{tag}/{filename} or /{tag}/{filename}.bin', { status: 404 });
    }

    try {
      let resolved = await resolveViaGithubCom(repo, tag, filename);
      if (!resolved && env.GITHUB_TOKEN) {
        resolved = await resolveViaApi(repo, tag, filename, env.GITHUB_TOKEN);
      }
      if (resolved && resolved.notFound) {
        return new Response('GitHub returned 404', { status: 404, headers: CORS });
      }
      if (!resolved) {
        return new Response('GitHub is unreachable right now. Try again in a minute.', {
          status: 502,
          headers: CORS,
        });
      }

      // Fetch the signed URL (reliable host), unless the API already
      // handed us the bytes directly.
      let assetResp = resolved.response;
      if (!assetResp) {
        for (let attempt = 1; attempt <= 3; attempt++) {
          assetResp = await fetch(resolved.location);
          if (assetResp.ok) break;
          if (attempt < 3) await new Promise(r => setTimeout(r, 300 * attempt));
        }
        if (!assetResp.ok) {
          return new Response(`GitHub returned ${assetResp.status}`, { status: 502, headers: CORS });
        }
      }

      // Return the binary with CORS headers
      return new Response(assetResp.body, {
        status: 200,
        headers: {
          ...CORS,
          'Content-Type': 'application/octet-stream',
          'Cache-Control': 'public, max-age=86400',
        }
      });
    } catch (err) {
      return new Response(`Proxy error: ${err.message}`, {
        status: 502,
        headers: CORS,
      });
    }
  }
};
