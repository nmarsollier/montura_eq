const fs = require('fs');
const path = require('path');

const src = path.resolve(__dirname, 'src');
const dist = path.resolve(__dirname, 'dist');

if (!fs.existsSync(dist)) fs.mkdirSync(dist, {recursive: true});

const alpine = fs.readFileSync(
    path.resolve(__dirname, 'node_modules/alpinejs/dist/cdn.min.js'), 'utf8');

const template = fs.readFileSync(path.join(src, 'index.html'), 'utf8');
const css = fs.readFileSync(path.join(src, 'style.css'), 'utf8');
const js = fs.readFileSync(path.join(src, 'app.js'), 'utf8');

const out = template
    .replace('<!--INLINE_ALPINE-->', `<script>${alpine}</script>`)
    .replace('<!--INLINE_CSS-->', `<style>${css}</style>`)
    .replace('<!--INLINE_JS-->', `<script>${js}</script>`);

fs.writeFileSync(path.join(dist, 'index.html'), out, 'utf8');
console.log('Built dist/index.html');

