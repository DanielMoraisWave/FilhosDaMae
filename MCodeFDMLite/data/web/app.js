// Scoreboard client. Connects to /ws and renders live score updates.

(function () {
  var ws, reconnectTimer;

  function setScore(id, val) {
    var el = document.getElementById(id);
    if (el.textContent !== String(val)) {
      el.textContent = val;
      el.classList.add('bump');
      setTimeout(function () { el.classList.remove('bump'); }, 220);
    }
  }

  function renderState(d) {
    setScore('s1', d.benfica);
    setScore('s2', d.sporting);
    var st = document.getElementById('status');
    if (d.state === 'negra') {
      st.className = 'status negra'; st.textContent = 'NEGRA!';
    } else if (d.state === 'benfica_wins') {
      st.className = 'status winner'; st.textContent = 'Benfica Ganhou!';
    } else if (d.state === 'sporting_wins') {
      st.className = 'status winner'; st.textContent = 'Sporting Grandioso!';
    } else {
      st.className = 'status hidden'; st.textContent = '';
    }
  }

  function connect() {
    ws = new WebSocket('ws://' + location.host + '/ws');
    ws.onopen  = function () {
      document.getElementById('dot').className = 'dot ok';
      clearTimeout(reconnectTimer);
    };
    ws.onclose = function () {
      document.getElementById('dot').className = 'dot';
      reconnectTimer = setTimeout(connect, 2000);
    };
    ws.onmessage = function (e) { renderState(JSON.parse(e.data)); };
  }

  window.sendReset = function () {
    if (ws && ws.readyState === 1) ws.send('reset');
  };

  connect();
})();
