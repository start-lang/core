(function(global) {
  const isNode = typeof process !== 'undefined' && process.versions != null && process.versions.node != null;

  global._wasm = null;

  function get_wasm() {
    return global._wasm.instance;
  }

  function get_memory() {
    return new Uint8Array(get_wasm().exports.memory.buffer);
  }

  async function WASM(wasmSrc, buildEnv) {
    global._wasm = null;
    global._args = [];

    const env = buildEnv(get_wasm, get_memory);

    const { input, output, errorOutput, outputc, errorOutputc } = env;

    let stdoutBuffer = new Uint8Array(1024);
    let stdoutBufferIndex = 0;
    let stderrBuffer = new Uint8Array(1024);
    let stderrBufferIndex = 0;
    let needs_input = 0;

    const decoder = new TextDecoder();
    const encoder = new TextEncoder();

    const fetchWasm = isNode ? fs.readFileSync(wasmSrc) : await fetch(wasmSrc).then(res => res.arrayBuffer());
    const obj = await WebAssembly.instantiate(fetchWasm, {
      env: {
        needs_input: function() { needs_input = 1; },
        get_now: () => (isNode ? performance.now() : Date.now()) * 1000,
        getargc: function() {
          return global._args.length + 1;
        },
        getarglen: function(index) {
          if (index === 0) {
            return wasmSrc.split('/').pop().length;
          }
          return global._args[index - 1].length;
        },
        getargstr: function(str, index) {
          let arg;
          if (index === 0) {
            arg = wasmSrc.split('/').pop();
          } else {
            arg = global._args[index - 1];
          }
          const memory = get_memory();
          const stringBuf = new Uint8Array(memory.buffer, str, arg.length + 1);
          for (let i = 0; i < arg.length; i++) {
            stringBuf[i] = encoder.encode(arg)[i];
          }
          stringBuf[arg.length] = 0;
          return str;
        },
        pechar: function(charCode) {
          if (errorOutputc) {
            errorOutputc(decoder.decode(new Uint8Array([charCode])));
            return charCode;
          }
          if (charCode === 10) {
            stderrBuffer[stderrBufferIndex++] = 0;
            errorOutput(decoder.decode(stderrBuffer.subarray(0, stderrBufferIndex)));
            stderrBufferIndex = 0;
          } else {
            stderrBuffer[stderrBufferIndex++] = charCode;
          }
          return charCode;
        },
        putchar: function(charCode) {
          if (outputc) {
            outputc(decoder.decode(new Uint8Array([charCode])));
            return charCode;
          }
          if (charCode === 10) {
            stdoutBuffer[stdoutBufferIndex++] = 0;
            output(decoder.decode(stdoutBuffer.subarray(0, stdoutBufferIndex)));
            stdoutBufferIndex = 0;
          } else {
            stdoutBuffer[stdoutBufferIndex++] = charCode;
          }
          return charCode;
        },
        print_string: function(str) {
          if (outputc) {
            outputc(decoder.decode(new Uint8Array([str])));
            return;
          }
          const memory = get_memory();
          let len = 0;
          while (memory[str + len] !== 0) len++;
          const stringBuf = new Uint8Array(memory.buffer, str, len);
          output(String.fromCharCode(...stringBuf));
        },
        ...env,
      }
    });
    global._wasm = obj;

    function run(args) {
      global._args = args;
      const { wasm_main } = get_wasm().exports;
      const r = wasm_main();
      return {
        stdout: decoder.decode(stdoutBuffer.subarray(0, stdoutBufferIndex)),
        stderr: decoder.decode(stderrBuffer.subarray(0, stderrBufferIndex)),
        code: r,
      };
    }

    function init(args) {
      global._args = args;
      const { wasm_init } = get_wasm().exports;
      const r = wasm_init();
      return {
        stdout: decoder.decode(stdoutBuffer.subarray(0, stdoutBufferIndex)),
        stderr: decoder.decode(stderrBuffer.subarray(0, stderrBufferIndex)),
        code: r,
      };
    }

    function step(input) {
      const { wasm_step } = get_wasm().exports;
      const r = wasm_step(input);
      return {
        stdout: decoder.decode(stdoutBuffer.subarray(0, stdoutBufferIndex)),
        stderr: decoder.decode(stderrBuffer.subarray(0, stderrBufferIndex)),
        code: r,
      };
    }

    function free() {
      const { wasm_free } = get_wasm().exports;
      wasm_free();
    }

    async function runAsync(args) {
      let r = 99;
      let c = 0;
      try {
        init(args);
        while (true) {
          if (needs_input === 1) {
            needs_input = 0;
            c = await input();
          } else {
            c = 0;
          }
          r = step(c);
          if (r.code !== 0) {
            break;
          } else if (c === 'q') {
            break;
          }
        }
        free();
      } catch (e) {
        console.error(e);
      }
      return {
        stdout: decoder.decode(stdoutBuffer.subarray(0, stdoutBufferIndex)),
        stderr: decoder.decode(stderrBuffer.subarray(0, stderrBufferIndex)),
        code: [0, 1].includes(r.code) ? 0 : r.code,
      };
    }

    return {
      run: run,
      init: init,
      step: step,
      free: free,
      runAsync: runAsync,
    };
  }

  if (isNode) {
    globalThis.require = require;
    globalThis.fs = require('fs');
    if (typeof require !== 'undefined' && require.main === module) {
      (async function() {
        if (process.stdin.isTTY) {
          process.stdin.setRawMode(true);
        }

        process.on('SIGINT', () => {
          if (process.stdin.isTTY) {
            process.stdin.setRawMode(false);
          }
          process.exit();
        });

        process.on('exit', () => {
          if (process.stdin.isTTY) {
            process.stdin.setRawMode(false);
          }
        });

        const w = await WASM(process.argv[2], (getWasm, getMemory) => {
          return {
            output: console.log,
            errorOutput: console.error,
            input: async function() {
              const buffer = Buffer.alloc(3);
              const fd = fs.openSync('/dev/tty', 'rs');
              const bytesRead = fs.readSync(fd, buffer, 0, 3);
              fs.closeSync(fd);
              if (bytesRead === 1) {
                return buffer[0];
              } else if (bytesRead > 1 && buffer[0] === 0x1b) {
                if (buffer[1] === 0x5b) {
                  switch (buffer[2]) {
                    case 0x41: return 0x01;
                    case 0x42: return 0x02;
                    case 0x43: return 0x03;
                    case 0x44: return 0x04;
                  }
                }
              }
              return 0x00;
            },
          };
        });

        const result = await w.runAsync(process.argv.slice(3));
        if (result.stdout) console.log(result.stdout);
        if (result.stderr) console.error(result.stderr);
        process.exit(result.code);
      })();
    } else {
      module.exports = { WASM };
    }
  } else {
    window.addEventListener('load', async function () {
      const ie = document.getElementById('interpreter');
      var term = null;

      if (ie) {
        const t = document.createElement('div');
        t.id = 'terminal';
        const b = document.createElement('div');
        b.id = 'border';
        b.appendChild(t);
        ie.appendChild(b);
        try {
          var baseTheme = {
            red:           '#CE5C5C',
            brightRed:     '#FF7272',
            green:         '#5BCC5B',
            brightGreen:   '#72FF72',
            yellow:        '#CCCC5B',
            brightYellow:  '#FFFF72',
            blue:          '#5D5DD3',
            brightBlue:    '#7279FF',
            magenta:       '#BC5ED1',
            brightMagenta: '#E572FF',
            cyan:          '#5DA5D5',
            brightCyan:    '#72F0FF',
            white:         '#F8F8F8',
            brightWhite:   '#FFFFFF',
          };
          term = new Terminal({
            fontFamily: 'monospace',
            fontSize: 14,
            theme: baseTheme,
            cursorStyle: 'underline',
          });
          term.open(document.getElementById('terminal'));
          document.getElementById('interpreter').style.display = 'flex';
          term.resize(131, 30);
          term.write("loading...\r");

          term.options.cursorBlink = false;
          term.options.theme = { cursor: '#000' };
        } catch (e) {
          console.error(e);
        }

        const w = await WASM("bin.wasm", (getWasm, getMemory) => {
          return {
            outputc: (c) => {
              if (c == "\n") {
                term.writeln("");
              } else {
                term.write(c);
              }
            },
            errorOutputc: (c) => {
              term.write(c);
              console.error(c);
            },
            input: async () => {
              return new Promise((resolve, reject) => {
                term.onKey(e => {
                  resolve(e.key.charCodeAt(0));
                });
              });
            },
          };
        });
        term.write("\x1b[2J\x1b[3J\x1b[H");

        async function runStarT(args) {
          console.log(args);
          term.focus();
          term.options.cursorBlink = true;
          term.options.theme = { cursor: '#fff' };
          const result = await w.runAsync(args);
          term.options.cursorBlink = false;
          term.options.theme = { cursor: '#000' };
          term.blur();
          if (result.stdout) term.writeln(result.stdout);
          if (result.stderr) term.writeln(result.stderr);
          if (result.code !== 0) {
            term.writeln(`exit code: ${result.code}`);
          }
          return result;
        }
        window.runStarT = runStarT;
      } else {
        function removeAnsi(str) {
          return str.replace(/\x1b\[[0-9;]*[a-zA-Z]/g, '');
        }
        const w = await WASM("bin.wasm", (getWasm, getMemory) => {
          return {
            output: (str) => console.log(removeAnsi(str)),
            errorOutput: (str) => console.log(removeAnsi(str)),
            input: async () => 0,
          };
        });
        async function runStarT(args) {
          let r = w.run(args);
          if (r.stdout) console.log(r.stdout);
          if (r.stderr) console.error(r.stderr);
          if (r.code !== 0) {
            console.log(`exit code: ${r.code}`);
          }
          return r;
        }
        window.runStarT = runStarT;
      }
      global.WASM = WASM;
    });
  }
  })(typeof window !== 'undefined' ? window : global);
