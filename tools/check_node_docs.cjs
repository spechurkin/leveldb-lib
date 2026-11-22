'use strict';
const fs = require('node:fs');
const path = require('node:path');
const parser = require('../bindings/node/node_modules/@babel/parser');
const root = path.resolve(__dirname, '..');
const declarations = fs.readdirSync(path.join(root, 'bindings/node/src'), {recursive: true})
    .filter(file => file.endsWith('.d.ts'))
    .map(file => path.join(root, 'bindings/node/src', file));
const missing = [];
let count = 0;

function inspect(node, comments, label) {
    count++;
    if (!(comments || node.leadingComments || []).some(c => c.type === 'CommentBlock' && c.value.startsWith('*'))) {
        missing.push(label);
    }
}

for (const declaration of declarations) {
    const source = parser.parse(fs.readFileSync(declaration, 'utf8'), {sourceType: 'module', plugins: ['typescript']});
    for (const statement of source.program.body) {
        const node = statement.declaration || statement;
        if (!['ClassDeclaration', 'TSInterfaceDeclaration', 'TSTypeAliasDeclaration'].includes(node.type)) continue;
        const name = node.id.name;
        inspect(node, statement.leadingComments || node.leadingComments, name);
        for (const member of node.body?.body || []) inspect(member, member.leadingComments, `${name}.${member.key?.name || member.kind}`);
    }
}
for (const name of ['Database', 'Batch', 'Cursor', 'Snapshot']) {
    const file = path.join(root, `build/docs/node/module-multileveldb-${name}.html`);
    if (!fs.readFileSync(file, 'utf8').includes('id="close"')) missing.push(`${name}.close in generated reference`);
}
if (missing.length) throw new Error(`Missing API documentation: ${missing.join(', ')}`);
console.log(`Node declarations: ${count} documented API items; all resource references include close().`);
