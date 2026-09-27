// Pocket SDR Web UI - Output Options page

const TYPES = ['NMEA', 'RTCM3', 'LOG', 'IF Data']; // type value = index + 1
const LOGS = ['TIME', 'POS', 'ATT', 'OBS', 'NAV', 'SAT', 'CH', 'EPH', 'ALM',
    'LOG', 'SENS'];
const NOTES = [
    'Keywords Replacement in Path',
    '%Y=Year(yyyy) %y=year(yy) %m=month(mm) %d=day(dd) %h=hour(00-23)',
    '%M=minute(00-59) %S=second(00-59)'
];

export class OutPage {
    constructor(app) {
        this.app = app;
        this.el = document.createElement('div');
        this.el.innerHTML =
            `<div class="toolbar">` +
            `<label class="ttl">Output Options</label>` +
            `<span class="space"></span>` +
            `<button id="ot-apply">Apply</button>` +
            `<button id="ot-reload">Reload</button>` +
            `</div>` +
            `<div class="cfg-body">` +
            `<div class="warn-txt cfg-warn" id="ot-note"></div>` +
            `<div class="cfg-frm"><div class="cfg-sec">Output Streams ` +
            `(File: local_path[::S=tint], TCP: [addr]:port)</div>` +
            [...Array(8)].map((_, i) =>
                `<div class="ot-str">` +
                `<label class="sig-chk"><input type="checkbox" ` +
                `class="ot-ena"> ${i + 1}</label>` +
                `<select class="ot-type">` + TYPES.map((t, j) =>
                    `<option value="${j + 1}">${t}</option>`).join('') +
                `</select>` +
                `<input type="text" class="ot-path"></div>`).join('') +
            `<div class="cfg-row"><label class="sig-chk">` +
            `<input type="checkbox" id="ot-array"> RF CH Separation (RTCM3)` +
            `</label></div></div>` +
            `<div class="cfg-frm">` +
            `<div class="cfg-sec">Output Receiver Log Types</div>` +
            `<div class="cfg-row"><span class="sig-grid">` + LOGS.map(t =>
                `<label class="sig-chk"><input type="checkbox" class="ot-log" ` +
                `data-log="${t}"> $${t}</label>`).join('') + `</span></div>` +
            `</div>` +
            `<div class="cfg-frm"><div class="cfg-note">` +
            NOTES.map(s => `<div>${s}</div>`).join('') + `</div></div>` +
            `</div>`;
        this.el.querySelector('#ot-apply').onclick = () => this.apply();
        this.el.querySelector('#ot-reload').onclick = () =>
            this.app.ws.get('cfg');
        for (const e of this.el.querySelectorAll('.ot-ena')) {
            e.onchange = () => this.updateEna();
        }
        app.ws.on('cfg', (msg) => {
            if (this.active) this.populate(msg);
        });
        app.ws.on('hello', () => {
            if (this.active) this.app.ws.get('cfg');
        });
    }
    // type and path follow the stream enable check ----------------------------
    updateEna() {
        const dis = this.el.querySelector('#ot-apply').disabled;
        this.el.querySelectorAll('.ot-str').forEach(row => {
            const ena = row.querySelector('.ot-ena').checked;
            row.querySelector('.ot-type').disabled = dis || !ena;
            row.querySelector('.ot-path').disabled = dis || !ena;
        });
    }
    setEditable(ena, run) {
        const dis = !ena || run;
        this.el.querySelector('#ot-apply').disabled = dis;
        for (const e of this.el.querySelectorAll('.cfg-body input, ' +
            '.cfg-body select')) {
            e.disabled = dis;
        }
        this.el.querySelector('#ot-note').textContent = !ena ?
            'Configuration not supported by the server' :
            run ? 'Stop the receiver to edit' : '';
        this.updateEna();
    }
    populate(cfg) {
        const types = cfg.types.split(',');
        const paths = cfg.paths.split('|');
        const logs = (cfg.log_mask || '').split(',');
        this.el.querySelectorAll('.ot-str').forEach((row, i) => {
            const type = parseInt(types[i]) || 0;
            row.querySelector('.ot-ena').checked = type > 0;
            row.querySelector('.ot-type').value = type > 0 ? type : 3;
            row.querySelector('.ot-path').value = paths[i] || '';
        });
        this.el.querySelectorAll('.ot-log').forEach(
            (e, i) => e.checked = logs[i] != '0');
        this.el.querySelector('#ot-array').checked = cfg.array_sep == 1;
        this.setEditable(cfg.ena, cfg.run);
    }
    apply() {
        const types = [...this.el.querySelectorAll('.ot-str')].map(row =>
            row.querySelector('.ot-ena').checked ?
            row.querySelector('.ot-type').value : '0').join(',');
        const paths = [...this.el.querySelectorAll('.ot-path')].map(
            e => e.value).join('|');
        const logs = [...this.el.querySelectorAll('.ot-log')].map(
            e => e.checked ? 1 : 0).join(',');
        this.app.ws.send({cmd: 'set_out', types: types, paths: paths,
            log_mask: logs,
            array_sep: this.el.querySelector('#ot-array').checked ? 1 : 0});
        this.app.msg('Output options applied.');
        this.app.ws.get('cfg');
    }
    show() {
        this.active = true;
        this.app.ws.get('cfg');
    }
    hide() {
        this.active = false;
    }
}
