// Pocket SDR Web UI - application shell

import {WsClient} from './ws.js';
import {RcvPage} from './pages/rcv.js';
import {RfchPage} from './pages/rfch.js';
import {BbchPage} from './pages/bbch.js';
import {CorrPage} from './pages/corr.js';
import {SatsPage} from './pages/sats.js';
import {SolPage} from './pages/sol.js';
import {ArrayPage} from './pages/array.js';
import {InpPage} from './pages/inp.js';
import {OutPage} from './pages/out.js';
import {SigPage} from './pages/sig.js';
import {OptsPage} from './pages/opts.js';
import {HelpPage} from './pages/help.js';
import {LogPage} from './pages/log.js';

// connection indicator icons (styled by the conn-on/off/run classes) ----------
const ICON_ON =
    '<svg width="13" height="13" viewBox="0 0 16 16" aria-hidden="true">' +
    '<circle cx="8" cy="8" r="3.4" fill="currentColor"/>' +
    '<path d="M3.4 3.4a6.5 6.5 0 0 0 0 9.2M12.6 3.4a6.5 6.5 0 0 1 0 9.2" ' +
    'fill="none" stroke="currentColor" stroke-width="1.6" ' +
    'stroke-linecap="round"/></svg>';
const ICON_OFF =
    '<svg width="13" height="13" viewBox="0 0 16 16" aria-hidden="true">' +
    '<circle cx="8" cy="8" r="5.2" fill="none" stroke="currentColor" ' +
    'stroke-width="1.6"/>' +
    '<path d="M4.3 11.7 11.7 4.3" stroke="currentColor" stroke-width="1.6" ' +
    'stroke-linecap="round"/></svg>';

const ws = new WsClient();

// shared application state ----------------------------------------------------
const app = {
    ws: ws,
    info: {nrfch: 1, narch: 0, nch: 0, fs: 0, name: 'Pocket SDR', ver: '',
        run: 0},
    msg: (s) => { document.getElementById('msg1').textContent = s; },
    selectCorrCh: (ch) => {} // set after pages are built
};

// connection indicator: WebSocket state, blinking while the receiver runs -----
function setConn(on) {
    const conn = document.getElementById('conn');
    const run = on && app.info.run;
    conn.innerHTML = on ? ICON_ON : ICON_OFF;
    conn.title = !on ? 'OFFLINE' : run ? 'CONNECTED (RUN)' :
        'CONNECTED (STOP)';
    conn.className = (on ? 'conn-on' : 'conn-off') + (run ? ' conn-run' : '');
}

// update shared info first, before any page hello handler runs ----------------
ws.on('hello', (msg) => {
    Object.assign(app.info, msg);
    document.getElementById('logo').title = msg.name + ' ver.' + msg.ver;
    document.getElementById('btn-start').disabled = !msg.cfg_ena || msg.run;
    document.getElementById('btn-stop').disabled = !msg.cfg_ena || !msg.run;
    setConn(true);
});

// pages -----------------------------------------------------------------------
const pages = [
    {name: 'Receiver', page: new RcvPage(app)},
    {name: 'RF CH', page: new RfchPage(app)},
    {name: 'BB CH', page: new BbchPage(app)},
    {name: 'Correlator', page: new CorrPage(app)},
    {name: 'Satellites', page: new SatsPage(app)},
    {name: 'Solution', page: new SolPage(app)},
    {name: 'Array', page: new ArrayPage(app)},
    {name: 'Input', page: new InpPage(app), notab: true},
    {name: 'Output', page: new OutPage(app), notab: true},
    {name: 'Signal', page: new SigPage(app), notab: true},
    {name: 'System', page: new OptsPage(app), notab: true},
    {name: 'Help', page: new HelpPage(app), notab: true},
    {name: 'Log', page: new LogPage(app)}
];
let cur = -1;

app.selectCorrCh = (ch) => {
    pages[3].page.setCh(ch);
};

// switch page (dir: 1 = swiped left, -1 = swiped right, 0 = no slide) --------
function selPage(i, dir = 0) {
    if (i == cur) return;
    if (cur >= 0) {
        pages[cur].page.hide();
        pages[cur].page.el.classList.remove('active', 'slide-l', 'slide-r');
        if (pages[cur].tab) pages[cur].tab.classList.remove('active');
    }
    cur = i;
    pages[cur].page.el.classList.add('active');
    if (dir) pages[cur].page.el.classList.add(dir > 0 ? 'slide-l' : 'slide-r');
    if (pages[cur].tab) {
        pages[cur].tab.classList.add('active');
        pages[cur].tab.scrollIntoView({block: 'nearest', inline: 'nearest'});
    }
    pages[cur].page.show();
}

// build tab bar and pages (notab: page opened by a title bar button) ----------
const tabbar = document.getElementById('tabbar');
const pagesEl = document.getElementById('pages');
pages.forEach((p, i) => {
    if (!p.notab) {
        const tab = document.createElement('div');
        tab.className = 'tab';
        tab.textContent = p.name;
        tab.onclick = () => selPage(i);
        tabbar.appendChild(tab);
        p.tab = tab;
    }
    p.page.el.classList.add('page');
    pagesEl.appendChild(p.page.el);
});

// left/right swipe on the page area switches to the previous/next tab page ---
const SWIPE_MIN = 40;    // minimum horizontal distance (px)
const SWIPE_SLOPE = 0.7; // maximum |dy/dx| accepted as horizontal
const SWIPE_SCRL = 0.3;  // over a scrolled table: fraction of the page width
const SWIPE_SCRL_MAX = 200; // and its upper limit (px)
const tabs = pages.map((p, i) => i).filter((i) => !pages[i].notab);
let swipe = null;

// nearest horizontally scrollable ancestor, which scrolls with the gesture ----
function hscroll(el) {
    for ( ; el && el != pagesEl; el = el.parentElement) {
        if (el.scrollWidth > el.clientWidth + 1 &&
            /auto|scroll/.test(getComputedStyle(el).overflowX)) return el;
    }
    return null;
}
pagesEl.addEventListener('touchstart', (ev) => {
    if (ev.touches.length > 1) { // pinch or the like is not a swipe
        swipe = null;
        return;
    }
    const el = hscroll(ev.target);
    swipe = {x: ev.touches[0].clientX, y: ev.touches[0].clientY, el: el,
        sl: el ? el.scrollLeft : 0};
}, {passive: true});

pagesEl.addEventListener('touchend', (ev) => {
    const s = swipe;
    swipe = null;
    if (!s) return;
    const dx = ev.changedTouches[0].clientX - s.x;
    const dy = ev.changedTouches[0].clientY - s.y;
    // a wide table (BB CH, Satellites, Log) scrolls with a short drag, while a
    // long swipe switches the page and takes the table back where it was
    const scrl = s.el && s.el.scrollLeft != s.sl;
    const min = !scrl ? SWIPE_MIN : Math.min(SWIPE_SCRL_MAX,
        Math.max(SWIPE_MIN, SWIPE_SCRL * pagesEl.clientWidth));
    if (Math.abs(dx) < min || Math.abs(dy) > SWIPE_SLOPE * Math.abs(dx)) return;
    const dir = dx < 0 ? 1 : -1, i = tabs.indexOf(cur) + dir;
    if (!tabs.includes(cur) || i < 0 || i >= tabs.length) return;
    if (scrl) s.el.scrollLeft = s.sl;
    selPage(tabs[i], dir);
}, {passive: true});

pagesEl.addEventListener('touchcancel', () => { swipe = null; });

// redraw the current page on resize (a canvas is scaled as an image otherwise)
let resizeTimer = null;
window.addEventListener('resize', () => {
    if (resizeTimer) return;
    resizeTimer = setTimeout(() => {
        resizeTimer = null;
        if (cur >= 0 && pages[cur].page.redraw) pages[cur].page.redraw();
    }, 100);
});

// WebSocket handlers ----------------------------------------------------------
ws.on('open', () => {
    setConn(true);
    app.msg('Connected to receiver.');
});
ws.on('close', () => {
    app.info.run = 0; // unknown until the next hello
    setConn(false);
    app.msg('Connection lost. Reconnecting...');
});
ws.on('rcv_stat', (msg) => {
    const f = msg.str.split(/\s+/);
    document.getElementById('msg2').textContent =
        (msg.cpu === undefined ? '' :
        'CPU: ' + msg.cpu.toFixed(1) + ' % ') + 'Time: ' + f[0] + ' s';
});
ws.on('ack', (msg) => {
    if (msg.cmd == 'start') { // the server reports the cause and the input
        app.msg((msg.ok ? 'Receiver started. ' : 'Receiver start error: ') +
            (msg.msg || ''));
    }
    else if (!msg.ok) {
        app.msg('Command error: ' + msg.cmd +
            (msg.msg ? ' (' + msg.msg + ')' : ''));
    }
    else if (msg.cmd == 'stop') app.msg('Receiver stopped.');
});
ws.on('error', (msg) => {
    app.msg('Error: ' + msg.msg);
});

// global subscriptions and start ----------------------------------------------
ws.on('open', () => {
    ws.sub('rcv_stat', {cyc: 200});
    ws.sub('pvt_sol', {cyc: 200}); // solution history collected page-wide
});
ws.connect();
// the receiver start takes time: show it is in progress until the ack --------
document.getElementById('btn-start').onclick = () => {
    document.getElementById('btn-start').disabled = true;
    app.msg('Receiver is starting ...');
    ws.send({cmd: 'start'});
};
document.getElementById('btn-stop').onclick = () => ws.send({cmd: 'stop'});
document.getElementById('btn-inp').onclick = () => selPage(7);
document.getElementById('btn-out').onclick = () => selPage(8);
document.getElementById('btn-sig').onclick = () => selPage(9);
document.getElementById('btn-sys').onclick = () => selPage(10);
document.getElementById('btn-help').onclick = () => selPage(11);
setConn(false);

const hash = ['receiver', 'rfch', 'bbch', 'corr', 'sats', 'sol', 'array',
    'inp', 'out', 'sig', 'opts', 'help', 'log'].indexOf(location.hash.slice(1));
selPage(hash < 0 ? 0 : hash);
