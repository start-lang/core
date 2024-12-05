(async function() {
  const { WASM } = require('./wasm_runtime.js');
  const w = await WASM("test.wasm", (getWasm, getMemory) => {
    return {
      output: console.log,
      errorOutput: console.error,
      input: () => 0,
    };
  });

  const result = w.run(process.argv.slice(3));
  if (result.stdout) console.log(result.stdout);
  if (result.stderr) console.error(result.stderr);
  process.exit(result.code);
}());