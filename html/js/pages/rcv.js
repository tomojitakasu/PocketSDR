// Pocket SDR Web UI - Receiver page

import {Plot, BG, FG, GR, LW, WARN, SYS_COLOR, SYS_COLOR2, cssVar, plotFont}
    from '../plot.js';

const D2R = Math.PI / 180;
// stream status -1 to 3: error, close, wait, connect, active
const LED_COLORS = [WARN, '#F8F8F8', '#EE9900', '#006600', '#00CC00'];
const SYSTEMS = ['ALL', 'GPS', 'GLONASS', 'Galileo', 'QZSS', 'BeiDou',
    'NavIC', 'SBAS'];
const SYS_ORDER = 'GREJCIS';

const ROWS_L = [ // id '!': title-only row, label '': value-only row
    ['time', 'Receiver Time (s)'],
    ['src', 'Input Source'],
    ['fmt', 'Fmt / # RF CH / # Array CH'],
    ['!', 'LO Frequencies (MHz)'],
    ['fo1', ''],
    ['fo2', ''],
    ['IQ', 'Sampling'],
    ['fs', 'Sampling Rate (Msps)'],
    ['nch', '# BB CH Locked/All'],
    ['rate', 'IF Data Rate (MB/s)'],
    ['buff', 'IF Data Buffer Usage (%)']
];
const ROWS_R = [
    ['stime', 'Time (GPST)'],
    ['sstat', 'Solution Status'],
    ['lat', 'Latitude (°)'],
    ['lon', 'Longitude (°)'],
    ['hgt', 'Altitude (m)'],
    ['rpy', 'Roll/Pitch/Yaw (°)'],
    ['nsat', '# Sats Used/All'],
    ['latency', 'Solution Latency (s)'],
    ['!', 'Output'],
    ['count', '# PVT/OBS/NAV'],
    ['sum', 'IF Data Log (MB)']
];

// jet colormap (v: 0-1) -------------------------------------------------------
function jetColor(v) {
    v = Math.min(Math.max(v, 0.0), 1.0);
    const c = (x) => Math.round(
        Math.min(Math.max(1.5 - Math.abs(x), 0.0), 1.0) * 255);
    return [c(4 * v - 3), c(4 * v - 2), c(4 * v - 1)];
}

// satellite system of satellite ID --------------------------------------------
export function satSys(sat) {
    return sat[0] >= '0' && sat[0] <= '9' ? 'S' : sat[0];
}

// satellite sort order --------------------------------------------------------
export function satNo(sat) {
    const sys = satSys(sat);
    const prn = parseInt(sat.replace(/^[A-Z]/, '')) || 0;
    return SYS_ORDER.indexOf(sys) * 1000 + prn;
}

export class RcvPage {
    constructor(app) {
        this.app = app;
        this.sigStat = {};  // sat -> [{sig, cn0}]
        this.satInfo = {};  // sat -> {az, el, pvt, ...}
        this.sats = [];
        this.satsKey = '';
        this.elMask = 15.0;
        this.el = document.createElement('div');
        const rows = (defs) => defs.map(([id, lbl]) =>
            id == '!' ?
            `<div class="rcv-row"><span class="lbl">${lbl}</span></div>` :
            `<div class="rcv-row">` +
            (lbl ? `<span class="lbl">${lbl}</span>` : '') +
            `<span class="val" id="rcv-${id}">---</span></div>`).join('');
        this.el.innerHTML =
            `<div class="toolbar">` +
            `<span class="space"></span>` +
            `<label id="rcv-gain-l" style="display:none">` +
            `<input type="checkbox" id="rcv-gain" checked> Gain</label>` +
            `<label>RF CH</label><select id="rcv-rf"><option>ALL</option>` +
            [...Array(16)].map((_, i) => `<option>${i+1}</option>`).join('') +
            `</select>` +
            `<label>Sys</label><select id="rcv-sys">` +
            SYSTEMS.map(s => `<option>${s}</option>`).join('') + `</select>` +
            `<label>Output</label><span id="rcv-leds">` +
            '<span class="led"></span>'.repeat(8) + `</span>` +
            `</div>` +
            `<div class="rcv-main">` +
            `<div class="rcv-top">` +
            `<div class="rcv-body2"><div class="rcv-grid">` +
            `<div class="rcv-col">${rows(ROWS_L)}</div>` +
            `<div class="rcv-col">${rows(ROWS_R)}</div>` +
            `</div></div>` +
            `<div class="plotbox rcv-sky"><canvas id="rcv-sky"></canvas>` +
            `</div></div>` +
            `<div class="plotbox rcv-cn0"><canvas id="rcv-cn0"></canvas>` +
            `</div></div>`;
        this.leds = this.el.querySelectorAll('#rcv-leds .led');
        this.sky = this.el.querySelector('#rcv-sky');
        this.cn0Plot = new Plot(this.el.querySelector('#rcv-cn0'), {
            margin: [15, 10, 15, 15], title: 'Signal C/N0 (dB-Hz)',
            ylim: [20, 55], ystep: 5, xticks: false});
        for (const id of ['rcv-rf', 'rcv-sys']) {
            this.el.querySelector('#' + id).onchange = () => this.resub();
        }
        app.ws.on('rcv_stat', (msg) => this.updateStat(msg));
        app.ws.on('pvt_sol', (msg) => this.updateSol(msg));
        app.ws.on('ch_stat', (msg) => this.updateChStat(msg));
        app.ws.on('sat_stat', (msg) => this.updateSatStat(msg));
        app.ws.on('array_stat', (msg) => {
            if (!this.active) return;
            this.arrayStat = msg.narch > 0 ? msg : null;
            this.el.querySelector('#rcv-gain-l').style.display =
                msg.narch > 0 ? '' : 'none';
            this.set('rpy', msg.narch > 0 ?
                msg.rpy.map(v => v.toFixed(3)).join(' / ') : '---');
            this.draw();
        });
        app.ws.on('opts', (msg) => {
            if (msg.el_mask !== undefined) this.elMask = msg.el_mask;
        });
        app.ws.on('hello', (msg) => { // clear the plots on receiver stop
            if (msg.run) return;
            this.sigStat = {};
            this.satInfo = {};
            this.sats = [];
            this.satsKey = '';
            if (this.active) this.draw();
        });
        this.el.querySelector('#rcv-gain').onchange = () => this.draw();
        this.sky.onclick = (ev) => this.onSkyClick(ev);
    }
    // selected array CH (0: none) ----------------------------------------------
    archSel() {
        const as = this.arrayStat;
        const arch = parseInt(this.el.querySelector('#rcv-rf').value) || 0;
        return as && as.narch > 0 && arch > as.nrfch &&
            arch <= as.nrfch + as.narch ? arch : 0;
    }
    onSkyClick(ev) {
        const arch = this.archSel();
        if (!arch) return;
        const r0 = this.sky.getBoundingClientRect();
        const cx = r0.width / 2, cy = r0.height / 2;
        const R = Math.min(r0.width, r0.height) / 2 - 16;
        const dx = (ev.clientX - r0.left - cx) / R;
        const dy = (cy - (ev.clientY - r0.top)) / R;
        const rr = Math.hypot(dx, dy);
        if (rr > 1.0) return;
        let az = Math.atan2(dx, dy) / D2R;
        if (az < 0.0) az += 360.0;
        const el = (1.0 - rr) * 90.0;
        this.app.ws.send({cmd: 'array_beam', rfch: arch, az: az, el: el});
        const beam = this.arrayStat.beams.find(b => b.ch == arch);
        if (beam) {
            beam.az = az;
            beam.el = el;
        }
        this.draw();
    }
    set(id, txt, warn) {
        const e = this.el.querySelector('#rcv-' + id);
        e.textContent = txt;
        e.classList.toggle('warn', !!warn);
    }
    resub() {
        if (!this.active) return;
        const rf = this.el.querySelector('#rcv-rf').value;
        this.app.ws.sub('ch_stat', {
            cyc: 200, sys: this.el.querySelector('#rcv-sys').value, chno: 0,
            min_lock: 2.0, rfch: rf == 'ALL' ? 0 : parseInt(rf), opt: 0});
    }
    updateStat(msg) {
        if (!this.active) return;
        const f = msg.str.split(/\s+/);
        if (f.length < 13) return;
        this.set('time', f[0]);
        this.set('src', f[1]);
        this.set('fmt', f[2]);
        this.set('fo1', f[3]);
        this.set('fo2', f[4]);
        this.set('IQ', f[5]);
        this.set('fs', f[6]);
        this.set('nch', f[7]);
        this.set('rate', f[8]);
        this.set('buff', f[9], parseFloat(f[9]) > 90.0);
        this.set('latency', f[10]);
        this.set('count', f[11]);
        this.set('sum', f[12], parseFloat(f[12]) > 0.0);
        for (let i = 0; i < 8; i++) {
            this.leds[i].style.background = LED_COLORS[(msg.strs[i] + 1)] ||
                LED_COLORS[1];
        }
    }
    updateSol(msg) {
        if (!this.active) return;
        const f = msg.str.split(/\s+/);
        if (f.length < 7) { // receiver stopped: initial state
            for (const id of ['stime', 'sstat', 'lat', 'lon', 'hgt', 'nsat']) {
                this.set(id, '---');
            }
            return;
        }
        this.set('stime', f[0] + ' ' + f[1]);
        this.set('sstat', f[6], false);
        this.set('lat', f[2]);
        this.set('lon', f[3]);
        this.set('hgt', f[4]);
        this.set('nsat', f[5]);
    }
    updateChStat(msg) {
        if (!this.active) return;
        this.sigStat = {};
        for (const line of msg.str.split('\n').slice(2)) {
            const f = line.trim().split(/\s+/);
            if (f.length < 16) continue;
            (this.sigStat[f[2]] = this.sigStat[f[2]] || []).push(
                {sig: f[3], cn0: parseFloat(f[6])});
        }
        this.sats = Object.keys(this.sigStat).sort(
            (a, b) => satNo(a) - satNo(b));
        const key = this.sats.join(',');
        if (key != this.satsKey) {
            this.satsKey = key;
            if (key) this.app.ws.sub('sat_stat', {sats: key, cyc: 200});
            else this.app.ws.unsub('sat_stat');
        }
        this.draw();
    }
    updateSatStat(msg) {
        if (!this.active) return;
        this.satInfo = {};
        for (const s of msg.sats) this.satInfo[s.sat] = s;
        this.draw();
    }
    draw() {
        this.drawSky();
        this.drawCn0();
    }
    redraw() { // on resize
        this.draw();
    }
    drawSky() {
        const cv = this.sky, dpr = window.devicePixelRatio || 1;
        const r0 = cv.getBoundingClientRect();
        if (r0.width < 60 || r0.height < 60) return;
        cv.width = Math.round(r0.width * dpr);
        cv.height = Math.round(r0.height * dpr);
        const ctx = cv.getContext('2d');
        ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
        const w = r0.width, h = r0.height;
        ctx.fillStyle = BG;
        ctx.fillRect(0, 0, w, h);
        const cx = w / 2, cy = h / 2, R = Math.min(w, h) / 2 - 12;
        const arch = this.archSel();
        if (arch && this.el.querySelector('#rcv-gain').checked) {
            this.drawGainOverlay(ctx, cx, cy, R, arch);
        }
        ctx.lineWidth = LW;
        for (const el of [0, 30, 60]) {
            ctx.strokeStyle = el == 0 ? FG : GR;
            ctx.beginPath();
            ctx.arc(cx, cy, R * (90 - el) / 90, 0, 2 * Math.PI);
            ctx.stroke();
        }
        ctx.strokeStyle = GR;
        for (let az = 0; az < 360; az += 30) {
            ctx.beginPath();
            ctx.moveTo(cx, cy);
            ctx.lineTo(cx + R * Math.sin(az * D2R),
                cy - R * Math.cos(az * D2R));
            ctx.stroke();
        }
        ctx.font = plotFont();
        ctx.fillStyle = FG;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        // azimuth labels just outside the ring, rotated to read radially
        ctx.textBaseline = 'bottom';
        for (let az = 0; az < 360; az += 30) {
            const lbl = az % 90 ? '' + az : 'NESW'[az / 90];
            ctx.save();
            ctx.translate(cx + R * Math.sin(az * D2R),
                cy - R * Math.cos(az * D2R));
            ctx.rotate(az * D2R);
            ctx.fillText(lbl, 0, -0.5);
            ctx.restore();
        }
        ctx.textBaseline = 'middle';
        const satR = parseFloat(cssVar('--sky-sat-size', '10')) || 10;
        ctx.font = cssVar('--sky-sat-font', '9px Tahoma, sans-serif');
        for (const sat of this.sats) {
            const si = this.satInfo[sat] || {az: 0, el: 0, pvt: 0};
            if (si.el <= 0.0) continue; // suppress invalid satellites
            const rr = R * (90 - si.el) / 90;
            const x = cx + rr * Math.sin(si.az * D2R);
            const y = cy - rr * Math.cos(si.az * D2R);
            const color = SYS_COLOR[satSys(sat)] || FG;
            const low = si.el < this.elMask;
            ctx.beginPath();
            ctx.arc(x, y, satR, 0, 2 * Math.PI);
            if (!low) { // low elevation: black edge, no fill
                ctx.fillStyle = si.pvt ? color : BG;
                ctx.fill();
            }
            ctx.strokeStyle = FG;
            ctx.stroke();
            ctx.fillStyle = si.pvt && !low ? BG : FG;
            ctx.fillText(sat, x, y);
        }
        if (arch) this.drawBeamMark(ctx, cx, cy, R, arch);
    }
    // overlay array gain heatmap (jet, -30 to +20 dB) --------------------------
    drawGainOverlay(ctx, cx, cy, R, arch) {
        const as = this.arrayStat;
        const beam = as.beams.find(b => b.ch == arch);
        if (!beam || !as.ant_pos) return;
        const [r, p, y] = as.rpy.map(v => v * D2R);
        const cr = Math.cos(r), sr = Math.sin(r);
        const cp = Math.cos(p), sp = Math.sin(p);
        const cy_ = Math.cos(y), sy = Math.sin(y);
        const Rm = [ // body-to-ENU rotation R = Rz(yaw) Ry(pitch) Rx(roll)
            [cy_*cp, cy_*sp*sr - sy*cr, cy_*sp*cr + sy*sr],
            [sy*cp,  sy*sp*sr + cy_*cr, sy*sp*cr - cy_*sr],
            [-sp,    cp*sr,             cp*cr]];
        const pos = [];
        as.ant_pos.forEach((q, i) => {
            if (!as.ant_ena[i]) return;
            pos.push([0, 1, 2].map(
                k => Rm[k][0]*q[0] + Rm[k][1]*q[1] + Rm[k][2]*q[2]));
        });
        if (pos.length < 1) return;
        const k = 2.0 * Math.PI / (299792458.0 / 1.57542e9);
        const azb = beam.az * D2R, elb = beam.el * D2R;
        const eb = [Math.sin(azb) * Math.cos(elb),
            Math.cos(azb) * Math.cos(elb), Math.sin(elb)];
        const projb = pos.map(q => eb[0]*q[0] + eb[1]*q[1] + eb[2]*q[2]);
        const M = 80;
        const img = new ImageData(M, M);
        for (let j = 0; j < M; j++) {
            const Y = 1.0 - 2.0 * j / (M - 1); // north
            for (let i = 0; i < M; i++) {
                const X = -1.0 + 2.0 * i / (M - 1); // east
                const R2 = X * X + Y * Y;
                if (R2 > 1.0) continue; // transparent outside
                const az = Math.atan2(X, Y);
                const el = Math.max(0.0, (1.0 - Math.sqrt(R2)) * Math.PI / 2);
                const ce = Math.cos(el);
                const e = [Math.sin(az) * ce, Math.cos(az) * ce, Math.sin(el)];
                let re = 0.0, im = 0.0;
                for (let a = 0; a < pos.length; a++) {
                    const ph = k * (e[0]*pos[a][0] + e[1]*pos[a][1] +
                        e[2]*pos[a][2] - projb[a]);
                    re += Math.cos(ph);
                    im += Math.sin(ph);
                }
                const gain = 20.0 * Math.log10(Math.hypot(re, im) + 1e-30);
                const [r_, g_, b_] = jetColor((gain + 30.0) / 50.0);
                const o = (j * M + i) * 4;
                img.data[o] = r_;
                img.data[o+1] = g_;
                img.data[o+2] = b_;
                img.data[o+3] = 200;
            }
        }
        if (!this.ovl) this.ovl = document.createElement('canvas');
        this.ovl.width = this.ovl.height = M;
        this.ovl.getContext('2d').putImageData(img, 0, 0);
        ctx.save();
        ctx.beginPath();
        ctx.arc(cx, cy, R, 0, 2 * Math.PI);
        ctx.clip();
        ctx.imageSmoothingEnabled = true;
        ctx.drawImage(this.ovl, cx - R, cy - R, 2 * R, 2 * R);
        ctx.restore();
    }
    // beam direction X mark ----------------------------------------------------
    drawBeamMark(ctx, cx, cy, R, arch) {
        const beam = this.arrayStat.beams.find(b => b.ch == arch);
        if (!beam || beam.el < 0.0 || beam.el > 90.0) return;
        const rr = R * (90.0 - beam.el) / 90.0;
        const x = cx + rr * Math.sin(beam.az * D2R);
        const y = cy - rr * Math.cos(beam.az * D2R);
        for (const [color, width] of [['#FFFFFF', 6], ['red', 2]]) {
            ctx.strokeStyle = color;
            ctx.lineWidth = width;
            ctx.beginPath();
            ctx.moveTo(x - 8, y - 8);
            ctx.lineTo(x + 8, y + 8);
            ctx.moveTo(x + 8, y - 8);
            ctx.lineTo(x - 8, y + 8);
            ctx.stroke();
        }
        ctx.lineWidth = 1;
    }
    // C/N0 bar (color null: untracked signal as a short blank bar) -------------
    drawCn0Bar(p, px, cn0, color) {
        const ctx = p.ctx;
        const py0 = p.yp(p.ylim[0]);
        const py1 = Math.min(color ? p.yp(cn0) : py0, py0 - 4);
        ctx.fillStyle = color || BG;
        ctx.fillRect(px - 2.2, py1, 4.4, py0 - py1);
        ctx.strokeStyle = FG;
        ctx.lineWidth = LW;
        ctx.strokeRect(px - 2.2, py1, 4.4, py0 - py1);
    }
    drawCn0() {
        const p = this.cn0Plot;
        const sys = this.el.querySelector('#rcv-sys').value;
        const sats = this.sats;
        const sigs = [...new Set([].concat( // signal slots of all satellites
            ...sats.map(s => this.sigStat[s].map(x => x.sig))))].sort();
        p.xlim = [-0.7, Math.max(sats.length, 1) - 0.3];
        p.begin();
        let nuse = 0;
        sats.forEach((sat, i) => {
            const si = this.satInfo[sat] || {pvt: 0};
            const color = (si.pvt ? SYS_COLOR : SYS_COLOR2)[satSys(sat)] ||
                FG;
            if (si.pvt) nuse++;
            if (sys == 'ALL') { // bars stacked (highest C/N0 first)
                for (const s of [...this.sigStat[sat]].sort(
                    (a, b) => b.cn0 - a.cn0)) {
                    this.drawCn0Bar(p, p.xp(i), s.cn0, color);
                }
            }
            else { // one slot per signal, side by side
                sigs.forEach((sig, j) => {
                    const s = this.sigStat[sat].find(x => x.sig == sig);
                    const px = p.xp(i) + (j - (sigs.length - 1) / 2) * 7;
                    this.drawCn0Bar(p, px, s ? s.cn0 : 0, s ? color : null);
                });
            }
        });
        p.end();
        sats.forEach((sat, i) => {
            p.textPx(p.xp(i), p.ax[3] + 2,
                sys == 'ALL' ? sat.replace(/^[A-Z]/, '') : sat,
                SYS_COLOR[satSys(sat)] || FG, 'center', 'top');
        });
        p.textPx(p.ax[0] + 8, p.ax[1] + 12,
            '#Sats: ' + nuse + '/' + sats.length, FG, 'left');
        if (sys == 'ALL') {
            const present = [...new Set(sats.map(satSys))];
            let px = p.ax[2] - 8;
            for (const s of present.reverse()) {
                p.textPx(px, p.ax[1] + 12, s, SYS_COLOR[s], 'right');
                px -= 7.5;
            }
        }
        else {
            p.textPx(p.ax[2] - 10, p.ax[1] + 12, 'Signals: ' + sigs.join(' '),
                FG, 'right');
        }
    }
    show() {
        this.active = true;
        this.satsKey = '';
        this.draw(); // redraw with the current (possibly empty) data
        this.resub();
        this.app.ws.sub('array_stat', {cyc: 500});
        this.app.ws.get('opts'); // for elevation mask
    }
    hide() {
        this.active = false;
        this.app.ws.unsub('ch_stat');
        this.app.ws.unsub('sat_stat');
        this.app.ws.unsub('array_stat');
    }
}
