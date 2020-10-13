
var grammar = ohm.grammarFromScriptElement();

var element = document.getElementById('output');
var codeelement = document.getElementById('code');
element.value = ''; // clear browser cache

var defColor = element.style.borderColor;
var codeFailed = false;

function print(text){
  text = text.replace(/\033\[[0-9;]*m/g,""); // remove ansi color codes
  element.value += text + "\n";
  element.scrollTop = element.scrollHeight; // focus on bottom
}

function printEmoji(code){
  let msg = element.value;
  let g = document.createElement('div');
  g.innerHTML = code;
  element.value = element.value + '' + g.innerHTML + ' ';
  delete(g);
}

var Module = {
  noInitialRun: true,
  noExitRuntime: true,
  preRun: [],
  postRun: [],
  print: function(text) {
    if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
    print(text);
  },
  printErr: function(text) {
    codeFailed = true;
    if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
    print('');
    printEmoji('&#x1F6A9');
    print(text);
  },
  setStatus: function(text) {
    if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
    if (text === Module.setStatus.last.text) return;
    let m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
    let now = Date.now();
    // if this is a progress update, skip it if too soon
    if (m && now - Module.setStatus.last.time < 30) return; 
    Module.setStatus.last.time = now;
    Module.setStatus.last.text = text;
    let status = text;
    let progress = '';
    element.value = status + '\n' + progress  + '\n';
    if (m) {
      text = m[1];
      progress = parseInt(m[2])*100 + '/' + parseInt(m[4])*100;
    } else {
      element.value = '';
    }
  },
  totalDependencies: 0,
  monitorRunDependencies: function(left) {
    this.totalDependencies = Math.max(this.totalDependencies, left);
    Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
  }
};
Module.setStatus('Downloading...');
window.onerror = function(event) {
  element.style.borderColor = 'red';
  Module.setStatus('Exception thrown, see JavaScript console');
  Module.setStatus = function(text) {
    if (text) Module.printErr('[post-exception status] ' + text);
  };
};

function run_code(){
  codeFailed = false;
  element.style.borderColor = defColor;
  let code = codeelement.value;

  print('');
  printEmoji('&#x1F680');
  print(code);

  let m = grammar.match(code);
  if (!m.succeeded()) {
      print('');
      printEmoji('&#x1F41E');
      print(m.message);
      element.style.borderColor = 'red';
      // return;
  }

  let result = Module.ccall('run', 'number', ['string'], [code]);
  let out = Module.ccall('get_buffer', 'string', [], []);
  Module._clean();
  print('');
  if (out) {
    print(out);
  }
  

  if (result != 0 || codeFailed){
    element.style.borderColor = 'red';
  }
}
