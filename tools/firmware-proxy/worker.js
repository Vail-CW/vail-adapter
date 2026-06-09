// Cloudflare Worker - CORS proxy for Vail GitHub release assets
// Deploy: npx wrangler deploy
//
// Routes (both supported):
//   /{tag}/{file}.bin                 -> Vail-CW/vail-summit  (legacy, unchanged)
//   /{repo}/{tag}/{file}.(bin|uf2|hex) -> Vail-CW/{repo}      (e.g. vail-adapter)
//
// Only Vail-CW repos and firmware extensions are allowed.

const ALLOWED_REPOS = ['vail-summit', 'vail-adapter'];

export default {
  async fetch(request) {
    // Handle CORS preflight
    if (request.method === 'OPTIONS') {
      return new Response(null, {
        headers: {
          'Access-Control-Allow-Origin': '*',
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

    const githubUrl = `https://github.com/Vail-CW/${repo}/releases/download/${tag}/${filename}`;

    try {
      const response = await fetch(githubUrl, { redirect: 'follow' });
      if (!response.ok) {
        return new Response(`GitHub returned ${response.status}`, {
          status: response.status,
          headers: { 'Access-Control-Allow-Origin': '*' }
        });
      }

      // Return the binary with CORS headers
      return new Response(response.body, {
        status: 200,
        headers: {
          'Access-Control-Allow-Origin': '*',
          'Content-Type': 'application/octet-stream',
          'Cache-Control': 'public, max-age=86400',
        }
      });
    } catch (err) {
      return new Response(`Proxy error: ${err.message}`, {
        status: 502,
        headers: { 'Access-Control-Allow-Origin': '*' }
      });
    }
  }
};
