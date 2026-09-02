// Pocket SDR Web UI - Solution page

import {Plot, BG, FG, GR, P1, P2, P3} from '../plot.js';

const D2R = Math.PI / 180;
const MAX_SOL = 3600;   // solution log size

// geodetic position to ECEF ---------------------------------------------------
function pos2ecef(lat, lon, hgt) {
    const a = 6378137.0, f = 1.0 / 298.257223563, e2 = f * (2.0 - f);
    const sB = Math.sin(lat * D2R), cB = Math.cos(lat * D2R);
    const sL = Math.sin(lon * D2R), cL = Math.cos(lon * D2R);
    const N = a / Math.sqrt(1.0 - e2 * sB * sB);
    return [(N + hgt) * cB * cL, (N + hgt) * cB * sL,
        (N * (1.0 - e2) + hgt) * sB];
}

// ECEF to local ENU relative to reference -------------------------------------
function ecef2enu(ref, r) {
    const d = [r[0] - ref.ecef[0], r[1] - ref.ecef[1], r[2] - ref.ecef[2]];
    const sB = Math.sin(ref.lat * D2R), cB = Math.cos(ref.lat * D2R);
    const sL = Math.sin(ref.lon * D2R), cL = Math.cos(ref.lon * D2R);
    return [
        -sL * d[0] + cL * d[1],
        -sB * cL * d[0] - sB * sL * d[1] + cB * d[2],
        cB * cL * d[0] + cB * sL * d[1] + sB * d[2]
    ];
}

export class SolPage {
    constructor(app) {
        this.app = app;
        this.log = [];
        this.ref = null;
        this.last = '';
        this.el = document.createElement('div');
        this.el.innerHTML =
            `<div class="toolbar">` +
            `<label>Type</label><select id="so-type">` +
            `<option>Pos ENU</option><option>Pos Horiz</option></select>` +
            `<span class="space"></span>` +
            `<button id="so-ref">Ref Pos</button>` +
            `<button id="so-clear">Clear</button>` +
            `<label>Span (s)</label><select id="so-span">` +
            ['60', '120', '300', '900', '1800', '3600'].map(
                v => `<option>${v}</option>`).join('') + `</select>` +
            `<label>Range (m)</label><select id="so-rng">` +
            ['0.1', '0.2', '0.5', '1', '2', '5', '10', '20', '50'].map(
                v => `<option>${v}</option>`).join('') + `</select>` +
            `</div>` +
            `<div class="sol-enu" id="so-enu">` +
            [0, 1, 2, 3].map(i => `<div class="plotbox">` +
                `<canvas id="so-plt${i}"></canvas></div>`).join('') +
            `</div>` +
            `<div class="sol-hori" id="so-hori">` +
            `<div class="plotbox"><canvas id="so-plt4"></canvas></div>` +
            `</div>`;
        this.el.querySelector('#so-span').value = '300';
        this.el.querySelector('#so-rng').value = '10';
        const titles = ['Pos E (m)', 'Pos N (m)', 'Pos U (m)', '# Sats'];
        this.plots = [0, 1, 2, 3].map(i => new Plot(
            this.el.querySelector('#so-plt' + i), {
            margin: [20, 15, i == 0 ? 15 : 4, i == 3 ? 15 : 4], taxis: 1,
            xlabels: i == 3})); // time labels only on the bottom panel
        this.horiPlot = new Plot(this.el.querySelector('#so-plt4'), {
            margin: [32, 15, 15, 24], // room for the axis labels
            xlabel: 'Pos E (m)', ylabel: 'Pos N (m)'});
        this.titles = titles;
        this.solStr = '';
        this.el.querySelector('#so-type').onchange = () => this.setMode();
        this.el.querySelector('#so-span').onchange = () => this.render();
        this.el.querySelector('#so-rng').onchange = () => this.render();
        this.el.querySelector('#so-clear').onclick = () => {
            this.log = [];
            this.render();
        };
        this.el.querySelector('#so-ref').onclick = () => {
            const cur = this.ref ?
                `${this.ref.lat} ${this.ref.lon} ${this.ref.hgt}` : '';
            const s = prompt('Reference position (lat lon hgt)', cur);
            if (!s) return;
            const v = s.trim().split(/\s+/).map(parseFloat);
            if (v.length >= 3 && v.every(x => !isNaN(x))) {
                this.ref = {lat: v[0], lon: v[1], hgt: v[2],
                    ecef: pos2ecef(v[0], v[1], v[2])};
                this.render();
            }
        };
        app.ws.on('pvt_sol', (msg) => this.updateSol(msg));
    }
    setMode() {
        const enu = this.el.querySelector('#so-type').value == 'Pos ENU';
        this.el.querySelector('#so-enu').style.display = enu ? '' : 'none';
        this.el.querySelector('#so-hori').style.display = enu ? 'none' : '';
        this.render();
    }
    updateSol(msg) { // solutions are collected even when the page is hidden
        const f = msg.str.split(/\s+/);
        if (f.length < 7) return;
        this.solStr = `${f[0]} ${f[1]} GPST  ${f[2]}°  ${f[3]}°  ` +
            `${f[4]} m  ${f[5]} ${f[6]}`;
        if (f[6] != 'FIX' || f[1] == this.last) {
            if (this.active) this.render();
            return;
        }
        this.last = f[1];
        const t = f[1].split(':');
        const tod = parseInt(t[0]) * 3600 + parseInt(t[1]) * 60 +
            parseFloat(t[2]);
        const lat = parseFloat(f[2]), lon = parseFloat(f[3]),
            hgt = parseFloat(f[4]);
        const ns = f[5].split('/');
        this.log.push({tod: tod, ecef: pos2ecef(lat, lon, hgt),
            ns: parseInt(ns[0]), nsat: parseInt(ns[1])});
        if (this.log.length > MAX_SOL) this.log.shift();
        if (!this.ref) {
            this.ref = {lat: lat, lon: lon, hgt: hgt,
                ecef: pos2ecef(lat, lon, hgt)};
        }
        this.render();
    }
    render() {
        if (!this.active) return;
        const span = parseFloat(this.el.querySelector('#so-span').value);
        const rng = parseFloat(this.el.querySelector('#so-rng').value);
        const enu = this.ref ?
            this.log.map(s => ecef2enu(this.ref, s.ecef)) : [];
        const t = this.log.map(s => s.tod);
        const n = this.log.length;
        if (this.el.querySelector('#so-type').value == 'Pos ENU') {
            const tend = n > 0 ? t[n-1] : 0.0;
            for (let i = 0; i < 4; i++) {
                const p = this.plots[i];
                p.opt.title = i == 0 ? this.solStr : ''; // solution as title
                p.xlim = [tend - span, tend + span * 0.001];
                if (i < 3) {
                    const c = n > 0 ? enu[n-1][i] : 0.0;
                    p.ylim = [c - rng, c + rng];
                }
                else {
                    p.ylim = [0, 80];
                }
                p.begin();
                if (i < 3) {
                    p.hline(0, P3);
                    const v = enu.map(e => e[i]);
                    p.line(t, v, GR);
                    p.dots(t, v, 1.5, P1);
                }
                else {
                    p.line(t, this.log.map(s => s.nsat), P2);
                    p.line(t, this.log.map(s => s.ns), P1);
                }
                p.end();
                if (n > 0) { // current value marks over the frame
                    if (i < 3) this.curMark(p, t[n-1], enu[n-1][i], P1);
                    else {
                        this.curMark(p, t[n-1], this.log[n-1].nsat, P2);
                        this.curMark(p, t[n-1], this.log[n-1].ns, P1);
                    }
                }
                p.textPx(p.ax[0] + 6, p.ax[1] + 10, this.titles[i], FG,
                    'left');
            }
        }
        else {
            const p = this.horiPlot;
            p.opt.title = this.solStr;
            p.resize(); // full width with equal E/N scale
            const [ml, mr, mt, mb] = p.opt.margin;
            const ratio = Math.max((p.w - ml - mr) / (p.h - mt - mb), 0.1);
            const ce = n > 0 ? enu[n-1][0] : 0.0; // center on the latest
            const cn = n > 0 ? enu[n-1][1] : 0.0;
            p.xlim = [ce - rng * ratio, ce + rng * ratio];
            p.ylim = [cn - rng, cn + rng];
            p.begin();
            p.vline(0, GR);
            p.hline(0, GR);
            const e = enu.map(v => v[0]), nn = enu.map(v => v[1]);
            p.line(e, nn, GR);
            p.dots(e, nn, 1.5, P1);
            p.end();
            if (n > 0) { // current value mark over the frame
                p.point(e[n-1], nn[n-1], 9, BG);
                p.point(e[n-1], nn[n-1], 7, P1);
            }
        }
    }
    // current value mark, kept within the y range ----------------------------
    curMark(p, x, y, color) {
        p.point(x, Math.min(Math.max(y, p.ylim[0]), p.ylim[1]), 7, color);
    }
    redraw() { // on resize
        this.render();
    }
    show() {
        this.active = true;
        this.setMode();
    }
    hide() {
        this.active = false;
    }
}
