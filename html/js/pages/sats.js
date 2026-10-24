// Pocket SDR Web UI - Satellites page

import {satSys, satNo} from './rcv.js';

const SYSTEMS = ['ALL', 'GPS', 'GLONASS', 'Galileo', 'QZSS', 'BeiDou',
    'NavIC', 'SBAS'];
const N_SIG = 12;   // number of signal column pairs

export class SatsPage {
    constructor(app) {
        this.app = app;
        this.sigStat = {};
        this.satInfo = {};
        this.sats = [];
        this.satsKey = '';
        this.el = document.createElement('div');
        let head = ['SAT', 'FCN', 'PVT', 'OBS', 'EPH', 'SVH', 'AZ(°)',
            'EL(°)'].map(h => `<th class="c">${h}</th>`).join('');
        for (let i = 1; i <= N_SIG; i++) {
            head += `<th class="c">SIG${i}</th><th>C/N0</th>`;
        }
        this.el.innerHTML =
            `<div class="toolbar">` +
            `<label>Sys</label><select id="st-sys">` +
            SYSTEMS.map(s => `<option>${s}</option>`).join('') + `</select>` +
            `<span class="space"></span>` +
            `<span class="mono" id="st-cnt"></span>` +
            `</div>` +
            `<div class="bbch-body"><table class="chtbl"><thead><tr>` +
            head + `</tr></thead><tbody id="st-rows"></tbody></table></div>`;
        this.rows = this.el.querySelector('#st-rows');
        this.el.querySelector('#st-sys').onchange = () => this.resub();
        app.ws.on('ch_stat', (msg) => this.updateChStat(msg));
        app.ws.on('sat_stat', (msg) => this.updateSatStat(msg));
    }
    resub() {
        if (!this.active) return;
        this.app.ws.sub('ch_stat', {
            cyc: 200, sys: this.el.querySelector('#st-sys').value, chno: 0,
            min_lock: 2.0, rfch: 0, opt: 0});
    }
    updateChStat(msg) {
        if (!this.active) return;
        this.sigStat = {};
        for (const line of msg.str.split('\n').slice(2)) {
            const f = line.trim().split(/\s+/);
            if (f.length < 16) continue;
            (this.sigStat[f[2]] = this.sigStat[f[2]] || []).push(
                {sig: f[3], cn0: f[6]});
        }
        this.sats = Object.keys(this.sigStat).sort(
            (a, b) => satNo(a) - satNo(b));
        const key = this.sats.join(',');
        if (key != this.satsKey) {
            this.satsKey = key;
            if (key) this.app.ws.sub('sat_stat', {sats: key, cyc: 200});
            else this.app.ws.unsub('sat_stat');
        }
        this.render();
    }
    updateSatStat(msg) {
        if (!this.active) return;
        this.satInfo = {};
        for (const s of msg.sats) this.satInfo[s.sat] = s;
        this.render();
    }
    render() {
        let html = '', nuse = 0, nsig = 0;
        for (const sat of this.sats) {
            const si = this.satInfo[sat] ||
                {az: 0, el: 0, pvt: 0, obs: 0, eph: 0, svh: 0, fcn: 0};
            const sigs = this.sigStat[sat];
            nsig += sigs.length;
            if (si.pvt) nuse++;
            // mask QZSS L1C/A, L1C/B health
            const svh = satSys(sat) == 'J' ? si.svh & 0xEE : si.svh;
            const cls = si.eph && svh ? 'unhealthy' :
                si.pvt ? '' : 'idle';
            let cells =
                `<td class="c">${sat}</td>` +
                `<td class="c">${satSys(sat) == 'R' ?
                    (si.fcn >= 0 ? '+' : '') + si.fcn : '-'}</td>` +
                `<td class="c">${si.pvt ? 'OK' : '-'}</td>` +
                `<td class="c">${si.obs ? 'OK' : '-'}</td>` +
                `<td class="c">${si.eph ? 'OK' : '-'}</td>` +
                `<td class="c">${si.eph ? ('0' + si.svh.toString(16)
                    .toUpperCase()).slice(-2) : '-'}</td>` +
                `<td>${si.az.toFixed(1)}</td>` +
                `<td>${si.el.toFixed(1)}</td>`;
            for (let i = 0; i < N_SIG; i++) {
                cells += i < sigs.length ?
                    `<td class="c">${sigs[i].sig}</td>` +
                    `<td>${sigs[i].cn0}</td>` :
                    `<td class="c">-</td><td>-</td>`;
            }
            html += `<tr class="${cls}">${cells}</tr>`;
        }
        this.rows.innerHTML = html;
        this.el.querySelector('#st-cnt').textContent =
            `# Sats Used/Tracked/Signals: ${nuse}/${this.sats.length}/${nsig}`;
    }
    show() {
        this.active = true;
        this.satsKey = '';
        this.resub();
    }
    hide() {
        this.active = false;
        this.app.ws.unsub('ch_stat');
        this.app.ws.unsub('sat_stat');
    }
}
