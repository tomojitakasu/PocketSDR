// Pocket SDR Web UI - BB CH page (channel status table)

const SYSTEMS = ['ALL', 'GPS', 'GLONASS', 'Galileo', 'QZSS', 'BeiDou',
    'NavIC', 'SBAS'];
const COLS = [ // [label, align]
    ['CH', ''], ['RF', 'c'], ['SAT', 'c'], ['SIG', 'c'], ['PRN', 'c'],
    ['LOCK(s)', ''], ['C/N0', ''], ['(dB-Hz)', 'l bar'], ['COFF(ms)', ''],
    ['DOP(Hz)', ''], ['ADR(cyc)', ''], ['SYNC', 'c'], ['#NAV', ''],
    ['#ERR', ''], ['#LOL', ''], ['FEC', '']
];

export class BbchPage {
    constructor(app) {
        this.app = app;
        this.el = document.createElement('div');
        this.el.innerHTML =
            `<div class="toolbar">` +
            `<label>RF CH</label><select id="bb-rfch"><option>ALL</option>` +
            [...Array(16)].map((_, i) => `<option>${i+1}</option>`).join('') +
            `</select>` +
            `<label>Sys</label><select id="bb-sys">` +
            SYSTEMS.map(s => `<option>${s}</option>`).join('') + `</select>` +
            `<select id="bb-state"><option>LOCK</option>` +
            `<option>ALL</option></select>` +
            `<span class="space"></span>` +
            `<span class="mono" id="bb-buff"></span>` +
            `<span class="mono" id="bb-srch"></span>` +
            `<span class="mono" id="bb-lock"></span>` +
            `</div>` +
            `<div class="bbch-body"><table class="chtbl"><thead><tr>` +
            COLS.map(c => `<th class="${c[1]}">${c[0]}</th>`).join('') +
            `</tr></thead><tbody id="bb-rows"></tbody></table></div>`;
        this.rows = this.el.querySelector('#bb-rows');
        for (const id of ['bb-rfch', 'bb-sys', 'bb-state']) {
            this.el.querySelector('#' + id).onchange = () => this.resub();
        }
        app.ws.on('ch_stat', (msg) => this.update(msg));
    }
    params() {
        const rfch = this.el.querySelector('#bb-rfch').value;
        const state = this.el.querySelector('#bb-state').value;
        return {
            cyc: 200, sys: this.el.querySelector('#bb-sys').value,
            chno: state == 'ALL' ? -1 : 0, min_lock: 2.0,
            rfch: rfch == 'ALL' ? 0 : parseInt(rfch), opt: 0
        };
    }
    resub() {
        if (this.active) this.app.ws.sub('ch_stat', this.params());
    }
    update(msg) {
        if (!this.active) return;
        const lines = msg.str.split('\n');
        const m = lines[0].match(
            /BUFF: *(\d+)% SRCH: *(\d+), *(\d+) LOCK: *(\d+)\/ *(\d+)/);
        if (m) {
            const buff = this.el.querySelector('#bb-buff');
            buff.textContent = 'BUFF: ' + m[1] + '%';
            buff.className = 'mono ' + (parseInt(m[1]) < 90 ? 'ok-txt' :
                'warn-txt');
            this.el.querySelector('#bb-srch').textContent =
                'SRCH: ' + m[2] + ',' + m[3];
            this.el.querySelector('#bb-lock').textContent =
                'LOCK: ' + m[4] + '/' + m[5];
            this.srch = [parseInt(m[2]), parseInt(m[3])];
        }
        let html = '';
        for (const line of lines.slice(2)) {
            let f = line.trim().split(/\s+/);
            if (f.length == 15) f.splice(7, 0, ''); // no C/N0 bar column
            if (f.length < 16) continue;
            const cn0 = parseFloat(f[6]);
            const bar = Math.min(Math.max((cn0 - 25.0) / 25.0, 0.0), 1.0) * 90;
            const cls = parseFloat(f[5]) <= 0.0 ? 'idle' :
                this.srch && this.srch.includes(parseInt(f[0])) ? 'srch' : '';
            const cells = f.map((v, i) => {
                if (i == 7) return `<td class="l bar"><span class="cn0bar" ` +
                    `style="width:${bar.toFixed(0)}%"></span></td>`;
                return `<td class="${COLS[i][1]}">${v}</td>`;
            });
            html += `<tr class="${cls}" data-ch="${f[0]}">` + cells.join('') +
                '</tr>';
        }
        this.rows.innerHTML = html;
        for (const tr of this.rows.querySelectorAll('tr')) {
            tr.onclick = () => this.app.selectCorrCh(parseInt(tr.dataset.ch));
        }
    }
    show() {
        this.active = true;
        this.app.ws.sub('ch_stat', this.params());
    }
    hide() {
        this.active = false;
        this.app.ws.unsub('ch_stat');
    }
}
