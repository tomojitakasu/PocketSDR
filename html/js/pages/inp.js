// Pocket SDR Web UI - Input Options page

const TYPES = [ // [label, SoapySDR driver] ('': Pocket SDR FE device)
    ['Pocket SDR FE', ''], ['USRP (uhd)', 'uhd'], ['LimeSDR (lime)', 'lime'],
    ['LimeSDR (limesuiteng)', 'limesuiteng'], ['BladeRF (bladerf)', 'bladerf'],
    ['RTL-SDR (rtlsdr)', 'rtlsdr'], ['Pluto-SDR (plutosdr)', 'plutosdr'],
    ['Airspy (airspy)', 'airspy']
];
const FMTS = ['INT8', 'INT8X2', 'RAW8', 'RAW16', 'RAW16I', 'RAW32', 'CS8',
    'CS16'];
const F_LO = [ // LO frequency selections (MHz, free input also allowed)
    '1176.450', '1191.795', '1202.025', '1207.140', '1227.600', '1246.000',
    '1248.060', '1268.520', '1278.750', '1561.098', '1575.420', '1602.000',
    '1600.995', '2492.028'];
const NOTES = [
    '* Automatically configured if &lt;Path&gt;.tag file exists.',
    '** -GAIN=gain_dB -BW=bandwidth_MHz'
];

export class InpPage {
    constructor(app) {
        this.app = app;
        this.el = document.createElement('div');
        const rfch = (ch) =>
            `<tr><td>${ch + 1}</td>` +
            `<td><input type="number" class="in-fo" step="any" ` +
            `list="in-fo-list"></td>` +
            `<td><select class="in-IQ"><option value="1">I</option>` +
            `<option value="2">IQ</option></select></td>` +
            `<td><select class="in-bits"><option>2</option>` +
            `<option>3</option></select></td>` +
            `<td><input type="number" class="in-lpf" step="any"></td>` +
            `</tr>`;
        this.el.innerHTML =
            `<div class="toolbar">` +
            `<label class="ttl">Input Options</label>` +
            `<span class="space"></span>` +
            `<button id="in-apply">Apply</button>` +
            `<button id="in-reload">Reload</button>` +
            `</div>` +
            `<div class="cfg-body">` +
            `<div class="warn-txt cfg-warn" id="in-note"></div>` +
            `<div class="cfg-row"><label>Input Source</label>` +
            `<select id="in-inp">` +
            `<option value="0">RF Frontend</option>` +
            `<option value="1">IF Data</option></select></div>` +
            `<div class="cfg-frm in-rf"><div class="cfg-sec">RF Frontend</div>` +
            `<div class="cfg-row"><label>Device Type</label>` +
            `<select id="in-type">` + TYPES.map(([label, drv]) =>
                `<option value="${drv}">${label}</option>`).join('') +
            `</select></div>` +
            `<div class="cfg-row"><label>Device Selection (Blank: Any)</label>` +
            `<label class="sub">USB Bus/Port</label>` +
            `<input type="number" id="in-bus" class="num">` +
            `<input type="number" id="in-port" class="num"></div>` +
            `<div class="cfg-row"><label>Device Configuration File</label>` +
            `<input type="checkbox" id="in-conf-ena">` +
            `<input type="text" id="in-conf" class="wide"></div></div>` +
            `<div class="cfg-frm in-if"><div class="cfg-sec">IF Data</div>` +
            `<div class="cfg-row">` +
            `<label>Path (file: local_path, addr:port: TCP client)</label>` +
            `<input type="text" id="in-file" class="wide"></div>` +
            `<div class="cfg-row"><label>Time Offset (s)</label>` +
            `<input type="number" id="in-toff" class="num" step="any">` +
            `<label class="sub">Time Scale</label>` +
            `<input type="number" id="in-tscale" class="num" step="any">` +
            `</div></div>` +
            `<div class="cfg-frm"><div class="cfg-sec">Channels</div>` +
            `<div class="cfg-row"><label>IF Data Format*</label>` +
            `<select id="in-fmt">` + FMTS.map((f, i) =>
                `<option value="${i+1}">${f}</option>`).join('') +
            `</select></div>` +
            `<div class="cfg-row"><label>Sampling Rate (Msps)*</label>` +
            `<input type="number" id="in-fs" class="num" step="any"></div>` +
            `<div class="cfg-row"><label>Device Options**</label>` +
            `<input type="text" id="in-devopt" class="wide"></div>` +
            `<div class="cfg-tbls">` +
            [0, 4].map(base =>
                `<table class="cfg-tbl"><thead><tr><th>RF_CH</th>` +
                `<th>LO_MHz*</th><th>I/IQ*</th><th>Bits*</th>` +
                `<th>LPF_MHz</th></tr></thead><tbody>` +
                [0, 1, 2, 3].map(i => rfch(base + i)).join('') +
                `</tbody></table>`).join('') +
            `</div></div>` +
            `<datalist id="in-fo-list">` +
            F_LO.map(f => `<option value="${f}">`).join('') + `</datalist>` +
            `<div class="cfg-note">` +
            NOTES.map(s => `<div>${s}</div>`).join('') + `</div>` +
            `</div>`;
        this.el.querySelector('#in-apply').onclick = () => this.apply();
        this.el.querySelector('#in-reload').onclick = () =>
            this.app.ws.get('cfg');
        this.el.querySelector('#in-inp').onchange = () => this.updateEna();
        this.el.querySelector('#in-conf-ena').onchange = () => this.updateEna();
        app.ws.on('cfg', (msg) => {
            if (this.active) this.populate(msg);
        });
        app.ws.on('hello', () => {
            if (this.active) this.app.ws.get('cfg');
        });
    }
    // RF Frontend and IF Data follow the input source --------------------------
    updateEna() {
        const dis = this.el.querySelector('#in-apply').disabled;
        const ifd = this.el.querySelector('#in-inp').value == '1';
        for (const e of this.el.querySelectorAll('.in-rf input, ' +
            '.in-rf select')) {
            e.disabled = dis || ifd;
        }
        for (const e of this.el.querySelectorAll('.in-if input')) {
            e.disabled = dis || !ifd;
        }
        this.el.querySelector('#in-conf').disabled = dis || ifd ||
            !this.el.querySelector('#in-conf-ena').checked;
    }
    setEditable(ena, run) {
        const dis = !ena || run;
        this.el.querySelector('#in-apply').disabled = dis;
        for (const e of this.el.querySelectorAll('.cfg-body input, ' +
            '.cfg-body select')) {
            e.disabled = dis;
        }
        this.el.querySelector('#in-note').textContent = !ena ?
            'Configuration not supported by the server' :
            run ? 'Stop the receiver to edit' : '';
        this.updateEna();
    }
    populate(cfg) {
        this.el.querySelector('#in-inp').value = cfg.inp == 1 ? 1 : 0;
        const type = this.el.querySelector('#in-type');
        const drv = cfg.inp == 0 ? '' : cfg.driver; // kept while on IF Data
        if (drv && ![...type.options].some(o => o.value == drv)) {
            type.add(new Option(drv, drv)); // keep a driver not in the list
        }
        type.value = drv;
        this.el.querySelector('#in-file').value = cfg.file;
        this.el.querySelector('#in-toff').value = cfg.toff;
        this.el.querySelector('#in-tscale').value = cfg.tscale;
        this.el.querySelector('#in-fmt').value = cfg.fmt || 2;
        this.el.querySelector('#in-fs').value = cfg.fs;
        this.el.querySelector('#in-bus').value = cfg.bus;
        this.el.querySelector('#in-port').value = cfg.port;
        this.el.querySelector('#in-conf').value = cfg.conf;
        this.el.querySelector('#in-conf-ena').checked = cfg.conf_ena == 1;
        this.el.querySelector('#in-devopt').value = cfg.dev_opt || '';
        const fo = cfg.fo.split(',');
        const IQ = cfg.IQ.split(',');
        const bits = cfg.bits.split(',');
        const lpf = (cfg.lpf || '').split(',');
        this.el.querySelectorAll('.in-fo').forEach(
            (e, i) => e.value = parseFloat(fo[i]) || 0);
        this.el.querySelectorAll('.in-IQ').forEach(
            (e, i) => e.value = IQ[i] || '2');
        this.el.querySelectorAll('.in-bits').forEach(
            (e, i) => e.value = bits[i] || '2');
        this.el.querySelectorAll('.in-lpf').forEach(
            (e, i) => e.value = parseFloat(lpf[i]) || 0);
        this.setEditable(cfg.ena, cfg.run);
    }
    apply() {
        const val = (id) => this.el.querySelector(id).value;
        const csv = (cls) => [...this.el.querySelectorAll(cls)].map(
            e => e.value || '0').join(',');
        const drv = val('#in-type');
        this.app.ws.send({cmd: 'set_inp',
            inp: val('#in-inp') == '1' ? 1 : (drv ? 2 : 0),
            file: val('#in-file'),
            toff: parseFloat(val('#in-toff')) || 0,
            tscale: parseFloat(val('#in-tscale')) || 1,
            fmt: parseInt(val('#in-fmt')),
            fs: parseFloat(val('#in-fs')) || 12,
            bus: parseInt(val('#in-bus')),
            port: parseInt(val('#in-port')),
            conf: val('#in-conf'),
            conf_ena: this.el.querySelector('#in-conf-ena').checked ? 1 : 0,
            dev_opt: val('#in-devopt'),
            driver: drv,
            fo: csv('.in-fo'), IQ: csv('.in-IQ'), bits: csv('.in-bits'),
            lpf: csv('.in-lpf')});
        this.app.msg('Input options applied.');
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
