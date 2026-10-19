# X Cleaner — Web

**English** · [한국어 / detailed original guide](README.ko.md)

Run `npm ci --cache .npm-cache`, then `npm run dev` from this directory. See `.env.example` for configuration names; actual credentials are excluded.

Archive parsing happens in the browser. Selection starts empty and execution is disabled by default. Preview the selection before connecting an account. Authentication and target verification are separate from explicit execution confirmation. Never treat demo data as input for real-account deletion.

For deployment, this directory (`projects/x-cleaner/deploy`) is the application root. Server-side secrets must not be exposed through client-side environment variables. Live OAuth, permissions, and deletion have not been validated by the portfolio cleanup.

The retained Korean guide documents cookie/token handling, selection checks, retries, and deployment settings. Review those details and the source before enabling live execution.

[Portfolio overview](../../../README.md) · [Attribution](../../../THIRD_PARTY_NOTICES.md)
