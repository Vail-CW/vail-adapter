// Cloudflare Worker - CORS proxy for Vail Summit GitHub release assets
// Deploy: npx wrangler deploy
// Only proxies .bin files from Vail-CW/vail-summit releases

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

    // Parse path: /{tag}/{filename} e.g. /v0.52/bootloader.bin
    const url = new URL(request.url);
    const match = url.pathname.match(/^\/(v[\d.]+)\/([\w.-]+\.bin)$/);
    if (!match) {
      return new Response('Not found. Use /{tag}/{filename}.bin', { status: 404 });
    }

    const [, tag, filename] = match;
    const githubUrl = `https://github.com/Vail-CW/vail-summit/releases/download/${tag}/${filename}`;

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
