// Pocket SDR Web UI - RF CH page (band map, PSD and histograms)

import {Plot, FG, GR, P1, P2, LW, SYS_COLOR, plotFont, plotTitleFont}
    from '../plot.js';

export const SIG_FREQ = { // signal carrier frequencies (MHz)
    L1CA: 1575.42, L1CB: 1575.42, L1CD: 1575.42, L1CP: 1575.42, L1S: 1575.42,
    L2CM: 1227.6, L5I: 1176.45, L5Q: 1176.45, L5SI: 1176.45, L5SQ: 1176.45,
    L5SIV: 1176.45, L5SQV: 1176.45, L6D: 1278.75, L6E: 1278.75,
    G1CA: 1602.0, G2CA: 1246.0, G1OCD: 1600.995, G1OCP: 1600.995,
    G2OCP: 1248.06, G3OCD: 1202.025, G3OCP: 1202.025,
    E1B: 1575.42, E1C: 1575.42, E5AI: 1176.45, E5AQ: 1176.45,
    E5ABI: 1191.795, E5ABQ: 1191.795, E5BI: 1207.14, E5BQ: 1207.14,
    E6B: 1278.75, E6C: 1278.75,
    B1I: 1561.098, B1CD: 1575.42, B1CP: 1575.42, B2AD: 1176.45,
    B2AP: 1176.45, B2I: 1207.14, B2BI: 1207.14, B3I: 1268.52,
    I1SD: 1575.42, I1SP: 1575.42, I5S: 1176.45, ISS: 2492.028
};
const HIST_XL = [5, 3, 5, 9, 9];    // histogram x range by bits
const HIST_YL = [0.4, 0.4, 0.4, 0.3, 0.25]; // histogram y range by bits

export class RfchPage {
    constructor(app) {
        this.app = app;
        this.bits = 2;
        this.nch = 0;
        this.sigs = {};   // RF CH -> [{sig, sys}]
        this.chs = [];    // rfch_stat entries
        this.psdMsg = {}; // last psd message by RF CH (to redraw on resize)
        this.histMsg = null; // last hist message
        this.el = document.createElement('div');
        this.el.innerHTML =
            `<div class="toolbar">` +
            `<label>RF CH</label><select id="rf-ch"></select>` +
            `<span class="space"></span>` +
            `<label>Filter BW (MHz)</label><select id="rf-bw">` +
            ['-', '2.5', '4.2', '8.7', '16.4', '23.4', '36.0'].map(
                v => `<option>${v}</option>`).join('') + `</select>` +
            `<select id="rf-order"><option>3rd</option><option>5th</option>` +
            `</select>` +
            `<label>LNA Gain</label><select id="rf-gain"><option>-</option>` +
            `<option>Auto</option>` + [...Array(64)].map(
                (_, i) => `<option>${i}</option>`).join('') + `</select>` +
            `<label>Ave (s)</label><select id="rf-tave">` +
            ['0.1', '0.03', '0.01', '0.003', '0.001'].map(
                v => `<option>${v}</option>`).join('') + `</select>` +
            `</div>` +
            `<div class="rfch-main" id="rf-single">` +
            `<div class="plotbox rfch-psd"><canvas id="rf-psd"></canvas></div>` +
            `<div class="plotbox"><canvas id="rf-hist1"></canvas></div>` +
            `<div class="plotbox"><canvas id="rf-hist2"></canvas></div>` +
            `</div>` +
            `<div class="rfch-band" id="rf-band">` +
            `<div class="plotbox"><canvas id="rf-band1"></canvas></div>` +
            `<div class="plotbox"><canvas id="rf-band2"></canvas></div>` +
            `</div>` +
            `<div class="rfch-tiles" id="rf-tiles">` +
            [0, 1, 2, 3].map(i => `<div class="plotbox">` +
                `<canvas id="rf-tile${i}"></canvas></div>`).join('') +
            `</div>`;
        this.el.querySelector('#rf-tave').value = '0.01';
        this.psdPlot = new Plot(this.el.querySelector('#rf-psd'), {
            margin: [20, 15, 15, 25], title: 'Power Spectral Density (dB/Hz)',
            xlabel: 'Frequency (MHz)', xlim: [1560, 1590], ylim: [-85, -45]
        });
        this.histPlot = [1, 2].map(i => new Plot(
            this.el.querySelector('#rf-hist' + i), {
            margin: [20, 15, 15, 25], title: 'Histogram ' + 'IQ'[i-1],
            xlabel: 'Quantized Value', xlim: [-5, 5], ylim: [0, 0.4]
        }));
        this.bandPlot = [1, 2].map(i => new Plot(
            this.el.querySelector('#rf-band' + i), {
            margin: [15, 15, 15, 15], xlim: i == 1 ? [1510, 1650] :
                [1160, 1300], ylim: [0, 9], yticks: false,
            title: i == 1 ? 'GNSS Signal Band L1 (MHz)' :
                'GNSS Signal Band L2/L5/L6 (MHz)'
        }));
        this.tilePlot = [0, 1, 2, 3].map(i => new Plot(
            this.el.querySelector('#rf-tile' + i), {
            margin: [20, 15, 15, 15], xlim: [1560, 1590], ylim: [-85, -45]
        }));
        this.el.querySelector('#rf-ch').onchange = () => this.resub();
        this.el.querySelector('#rf-tave').onchange = () => this.resub();
        this.el.querySelector('#rf-gain').onchange = () => this.setGain();
        this.el.querySelector('#rf-bw').onchange = () => this.setFilt();
        this.el.querySelector('#rf-order').onchange = () => this.setFilt();
        app.ws.on('psd', (msg) => this.updatePsd(msg));
        app.ws.on('hist', (msg) => this.updateHist(msg));
        app.ws.on('ch_stat', (msg) => this.updateSigs(msg));
        app.ws.on('rfch_stat', (msg) => {
            if (!this.active) return;
            this.chs = msg.chs;
            if (this.mode() == 'band') this.drawBand();
        });
        app.ws.on('hello', (msg) => {
            this.fillChs();
            if (!msg.run) { // clear the plots on receiver stop
                this.sigs = {};
                this.chs = [];
                this.psdMsg = {};
                this.histMsg = null;
                if (this.active) this.clear();
            }
            if (this.active) {
                this.resub();
                this.app.ws.get('ch_stat', {chno: -1});
            }
        });
    }
    fillChs() {
        const sel = this.el.querySelector('#rf-ch');
        const n = this.app.info.nrfch + this.app.info.narch;
        if (this.nch == n) return;
        this.nch = n;
        let opts = ['ALL'];
        for (let i = 1; i <= n; i++) opts.push('' + i);
        for (let i = 1; i < n; i += 4) opts.push(i + '-' + (i + 3));
        sel.innerHTML = opts.map(v => `<option>${v}</option>`).join('');
        sel.value = '1';
    }
    mode() {
        const v = this.el.querySelector('#rf-ch').value;
        return v == 'ALL' ? 'band' : v.includes('-') ? 'tiles' : 'single';
    }
    rfch() {
        return parseInt(this.el.querySelector('#rf-ch').value) || 1;
    }
    resub() {
        if (!this.active) return;
        const mode = this.mode();
        const tave = parseFloat(this.el.querySelector('#rf-tave').value);
        for (const id of ['rf-single', 'rf-band', 'rf-tiles']) {
            this.el.querySelector('#' + id).style.display = 'none';
        }
        if (mode == 'single') {
            this.el.querySelector('#rf-single').style.display = '';
            this.app.ws.sub('psd', {rfch: this.rfch(), cyc: 100, tave: tave,
                nfft: 2048});
            this.app.ws.sub('hist', {rfch: this.rfch(), cyc: 200, tave: tave});
            this.app.ws.unsub('rfch_stat');
        }
        else if (mode == 'band') {
            this.el.querySelector('#rf-band').style.display = '';
            this.app.ws.sub('rfch_stat', {cyc: 500});
            this.app.ws.unsub('psd');
            this.app.ws.unsub('hist');
        }
        else {
            this.el.querySelector('#rf-tiles').style.display = '';
            this.app.ws.sub('psd', {rfch: 0, cyc: 200, tave: tave,
                nfft: 2048});
            this.app.ws.unsub('hist');
            this.app.ws.unsub('rfch_stat');
        }
    }
    setGain() {
        const val = this.el.querySelector('#rf-gain').value;
        if (val == '-' || this.mode() != 'single') return;
        this.app.ws.send({cmd: 'set_gain', rfch: this.rfch(),
            gain: val == 'Auto' ? 0 : parseInt(val) + 1});
    }
    setFilt() {
        const bw = this.el.querySelector('#rf-bw').value;
        const order = this.el.querySelector('#rf-order').value;
        if (bw == '-' || this.mode() != 'single') return;
        this.app.ws.send({cmd: 'set_filt', rfch: this.rfch(),
            bw: parseFloat(bw), freq: 0.0, order: order == '3rd' ? 1 : 0});
    }
    updateSigs(msg) { // derive signal list per RF CH (one-shot get)
        if (!this.active) return;
        this.sigs = {};
        for (const line of msg.str.split('\n').slice(2)) {
            const f = line.trim().split(/\s+/);
            if (f.length < 16) continue;
            const rf = parseInt(f[1]), sig = f[3];
            const sys = f[2][0] >= '0' && f[2][0] <= '9' ? 'S' : f[2][0];
            this.sigs[rf] = this.sigs[rf] || [];
            if (!this.sigs[rf].find(s => s.sig == sig)) {
                this.sigs[rf].push({sig: sig, sys: sys});
            }
        }
        if (this.mode() == 'band') this.drawBand();
    }
    // draw PSD into a plot (used by single view and tiles)
    drawPsdPlot(p, msg, marks) {
        const fo = msg.fo * 1e-6, fs = msg.fs * 1e-6;
        p.xlim = msg.IQ == 1 ? [fo, fo + fs / 2] : [fo - fs / 2, fo + fs / 2];
        p.ylim = [-85, -45];
        p.begin();
        const x = new Array(msg.n), y = new Array(msg.n);
        for (let i = 0; i < msg.n; i++) {
            x[i] = p.xlim[0] + (p.xlim[1] - p.xlim[0]) * i / (msg.n - 1);
            y[i] = msg.psd[i];
        }
        p.vline(fo, GR);
        p.line(x, y, P1);
        if (marks) {
            let dy = 14;
            for (const s of this.sigs[msg.rfch] || []) {
                const f = SIG_FREQ[s.sig];
                if (!f || f < p.xlim[0] || f > p.xlim[1]) continue;
                const color = SYS_COLOR[s.sys] || FG;
                p.markPx(p.xp(f), p.ax[1] + dy, 8, color, 1);
                p.textPx(p.xp(f) + 7, p.ax[1] + dy, s.sig, color, 'left',
                    'middle');
                dy += 11;
            }
        }
        p.end();
        // LO frequency mark with the label on its IQ: right / I: left side
        const fx = p.xp(fo), fy = p.ax[3] - 10;
        p.markPx(fx, fy, 8, FG, 1);
        p.textPx(fx + (msg.IQ == 1 ? 7 : -7), fy, fo.toFixed(3) + ' MHz', FG,
            msg.IQ == 1 ? 'left' : 'right', 'middle');
        p.textPx(p.ax[2] - 10, p.ax[3] - 10,
            (msg.IQ == 1 ? 'I' : 'IQ') + ' (' + msg.bits + ' bits)', FG,
            'right', 'middle');
        p.textPx(p.ax[0] + 10, p.ax[1] + 15, 'CH' + msg.rfch, P1, 'left',
            'middle', plotTitleFont());
        p.textPx(p.ax[2] - 10, p.ax[1] + 15, 'F_S: ' + fs.toFixed(3) + ' MHz',
            FG, 'right', 'middle');
    }
    updatePsd(msg) {
        if (!this.active) return;
        this.psdMsg[msg.rfch] = msg;
        const mode = this.mode();
        if (mode == 'single' && msg.rfch == this.rfch()) {
            this.bits = msg.bits;
            this.drawPsdPlot(this.psdPlot, msg, true);
        }
        else if (mode == 'tiles') {
            const start = parseInt(this.el.querySelector('#rf-ch').value);
            const i = msg.rfch - start;
            if (i >= 0 && i < 4) this.drawPsdPlot(this.tilePlot[i], msg,
                false);
        }
    }
    updateHist(msg) {
        if (!this.active) return;
        this.histMsg = msg;
        if (this.mode() != 'single' || msg.rfch != this.rfch()) return;
        const b = Math.min(this.bits, 4);
        for (const i of [0, 1]) {
            const p = this.histPlot[i];
            const hist = i == 0 ? msg.hist1 : msg.hist2;
            p.xlim = [-HIST_XL[b], HIST_XL[b]];
            p.ylim = [0, HIST_YL[b]];
            p.begin();
            let ave = 0.0, vari = 0.0;
            if (msg.IQ == 2 || i == 0) {
                for (let j = 0; j < msg.val.length; j++) {
                    ave += msg.val[j] * hist[j];
                }
                for (let j = 0; j < msg.val.length; j++) {
                    vari += (msg.val[j] - ave) ** 2 * hist[j];
                    p.bar(msg.val[j], hist[j], p.xs * 0.6, P1);
                }
            }
            p.end();
            p.textPx(p.ax[2] - 8, p.ax[1] + 12, 'Ave: ' + ave.toFixed(2),
                FG, 'right', 'middle');
            p.textPx(p.ax[2] - 8, p.ax[1] + 24,
                'Std: ' + Math.sqrt(vari).toFixed(2), FG, 'right', 'middle');
        }
    }
    // redraw the current view from the last received data ---------------------
    redraw() { // on resize
        this.clear(); // axes only where no data has arrived yet
        for (const rfch in this.psdMsg) this.updatePsd(this.psdMsg[rfch]);
        if (this.histMsg) this.updateHist(this.histMsg);
    }
    // draw all plots of the current view empty (axes only) --------------------
    clear() {
        const mode = this.mode();
        if (mode == 'single') {
            for (const p of [this.psdPlot, ...this.histPlot]) {
                p.begin();
                p.end();
            }
        }
        else if (mode == 'band') this.drawBand();
        else {
            for (const p of this.tilePlot) {
                p.begin();
                p.end();
            }
        }
    }
    drawBand() {
        const SYS_NAME = {G: 'GPS', R: 'GLONASS', E: 'Galileo', J: 'QZSS',
            C: 'BeiDou', I: 'NavIC', S: 'SBAS'};
        this.bandPlot.forEach((p, pi) => {
            const band = (c) => { // channel band (MHz)
                const fo = c.fo * 1e-6, fs = c.fs * 1e-6;
                return [c.IQ == 1 ? fo : fo - fs / 2, fo + fs / 2];
            };
            p.ylim = [0, 1];
            p.begin();
            const ctx = p.ctx;
            const boxes = [];
            for (const c of this.chs) {
                const [lo, hi] = band(c);
                if (hi < p.xlim[0] || lo > p.xlim[1]) continue;
                boxes.push({ch: c, lo: lo, hi: hi,
                    x0: p.xp(Math.max(lo, p.xlim[0])),
                    x1: p.xp(Math.min(hi, p.xlim[1]))});
            }
            const py0 = p.yp(0.87), py1 = p.yp(0.05);
            // fills, frames and texts in separate passes so that a CH box
            // overlapping another one does not hide its frame or texts
            ctx.fillStyle = '#F4F4F2';
            for (const b of boxes) {
                ctx.fillRect(b.x0, py0, b.x1 - b.x0, py1 - py0);
            }
            ctx.strokeStyle = FG;
            for (const b of boxes) {
                ctx.strokeRect(b.x0 + 0.5, py0 + 0.5, b.x1 - b.x0 - 1,
                    py1 - py0 - 1);
            }
            for (const b of boxes) {
                const xc = (b.x0 + b.x1) / 2;
                p.textPx(xc, py0 - 8, 'CH' + b.ch.ch, P1, 'center', 'middle',
                    plotTitleFont());
                p.textPx(xc, py1 - 9, (b.ch.fo * 1e-6).toFixed(3), FG,
                    'center', 'middle');
                let k = 0;
                for (const s of this.sigs[b.ch.ch] || []) {
                    const f = SIG_FREQ[s.sig];
                    if (!f || f < b.lo || f > b.hi) continue;
                    const color = SYS_COLOR[s.sys] || FG;
                    const py = py0 + 12 + (k++) * 11;
                    const left = p.xp(f) > xc;
                    p.markPx(p.xp(f), py, 7, color, 1);
                    p.textPx(p.xp(f) + (left ? -6 : 6), py, s.sig, color,
                        left ? 'right' : 'left', 'middle');
                }
            }
            p.end();
            if (pi == 0) { // system color legend (vertical, top right)
                const present = 'GREJCIS'.split('').filter(s => [].concat(
                    ...Object.values(this.sigs)).some(e => e.sys == s));
                present.forEach((s, k) => {
                    const py = p.ax[1] + 14 + k * 13;
                    p.markPx(p.ax[2] - 72, py, 7, SYS_COLOR[s] || FG, 1);
                    p.textPx(p.ax[2] - 64, py, SYS_NAME[s], SYS_COLOR[s] ||
                        FG, 'left', 'middle');
                });
            }
        });
    }
    show() {
        this.active = true;
        this.fillChs();
        this.clear(); // no stale plots until the first data arrives
        this.resub();
        this.app.ws.get('ch_stat', {chno: -1}); // for signal marks
    }
    hide() {
        this.active = false;
        this.app.ws.unsub('psd');
        this.app.ws.unsub('hist');
        this.app.ws.unsub('rfch_stat');
    }
}
