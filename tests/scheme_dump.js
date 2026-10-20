// Emits the same scheme dump as tests/scheme_dump.cpp, but from the
// JavaScript colour engine in docs/themes.html, so the two can be diffed.
//
// Run: node tests/scheme_dump.js
'use strict';

const fs = require('fs');
const path = require('path');

const html = fs.readFileSync(path.join(__dirname, '..', 'docs', 'themes.html'), 'utf8');
const script = html.split('<script>')[1].split('</script>')[0];
// Take only the colour engine, drop the page's UI glue.
const engine = script.split('// ---------------------------------------------------------------- UI glue')[0];
const { makeScheme } = new Function(`${engine}return { makeScheme };`)();

// Must match kRoles in tests/scheme_dump.cpp.
const ROLES = ['primary', 'onPrimary', 'primaryContainer', 'onPrimaryContainer', 'surface',
               'onSurface', 'error', 'success', 'warning', 'outline', 'tertiary', 'secondary',
               'surfaceVariant', 'inversePrimary', 'surfaceContainer', 'onSurfaceVariant'];

const hex = c => ((c.r << 16) | (c.g << 8) | c.b).toString(16).padStart(6, '0').toUpperCase();

const SEEDS = ['6750A4', '00497F', '006C4D', '7A3045', '7C5800', '4C3B6F', '5A5A66', '0B57D0',
               '00639B'];
const CONTRASTS = [-0.3, 0, 0.1, 0.2, 0.3];

const lines = [];
for (const seed of SEEDS) {
  for (const dark of [0, 1]) {
    for (const contrast of CONTRASTS) {
      for (const vibrant of [0, 1]) {
        const s = makeScheme(seed, !!dark, contrast, !!vibrant);
        const missing = ROLES.filter(r => !s[r]);
        if (missing.length) {
          throw new Error(`themes.html engine does not produce: ${missing.join(', ')}`);
        }
        lines.push([seed, dark, contrast.toFixed(1), vibrant].concat(ROLES.map(r => hex(s[r])))
                     .join('|'));
      }
    }
  }
}
process.stdout.write(`${lines.join('\n')}\n`);
