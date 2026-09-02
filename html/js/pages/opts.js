// Pocket SDR Web UI - Options page (runtime system options)

const B_LOOP = ['1.0', '2.5', '5.0', '7.5', '10.0', '15.0', '20.0'];
const T_INTG = ['0.005', '0.01', '0.02', '0.05', '0.1', '0.2'];

const OPTS = [ // [key, label, selections (free input also allowed)]
    ['epoch', 'Epoch Interval for PVT (s)',
        ['0.1', '0.2', '0.5', '1.0', '2.0', '5.0']],
    ['lag_epoch', 'Max Epoch Lag for PVT (s)',
        ['0.1', '0.2', '0.5', '1.0', '2.0']],
    ['el_mask', 'Elevation Mask for PVT (°)',
        ['5', '10', '15', '20', '25', '30']],
    ['sp_corr', 'Correlator Spacing (chip)',
        ['0.05', '0.1', '0.15', '0.2', '0.25', '0.5', '0.75', '1.0']],
    ['t_acq', 'Integration Time for Acquisition (s)',
        ['0.01', '0.02', '0.05', '0.1', '0.2']],
    ['t_acq_ext', 'Integration Time for Assisted Acq (s)',
        ['0.01', '0.02', '0.05', '0.1', '0.2']],
    ['t_dll', 'Integration Time for DLL (s)', T_INTG],
    ['t_coh', 'Coherent Integration for Pilot (s)', T_INTG],
    ['b_dll', 'DLL Loop Filter Bandwidth (Hz)',
        ['0.1', '0.2', '0.3', '0.4', '0.5', '0.75', '1.0']],
    ['b_pll', 'PLL Loop Filter Bandwidth (Hz)', B_LOOP],
    ['b_fll_w', 'FLL Loop Filter Bandwidth Wide (Hz)', B_LOOP],
    ['b_fll_n', 'FLL Loop Filter Bandwidth Narrow (Hz)', B_LOOP],
    ['max_dop', 'Max Doppler Freq to Search (Hz)',
        ['3000', '5000', '7000', '10000', '15000', '20000']],
    ['thres_cn0_l', 'C/N0 Threshold for Lock (dB-Hz)',
        ['30.0', '31.0', '32.0', '33.0', '34.0', '35.0', '36.0']],
    ['thres_cn0_u', 'C/N0 Threshold for Lost (dB-Hz)',
        ['18.0', '19.0', '20.0', '21.0', '22.0', '23.0', '24.0', '25.0',
            '27.0', '30.0']],
    ['thres_cn0_ext', 'C/N0 Threshold for Assisted Acq (dB-Hz)',
        ['25.0', '26.0', '27.0', '28.0', '29.0', '30.0', '31.0', '32.0']],
    ['thres_pli', 'Carrier Lock Threshold (PLI)',
        ['0.1', '0.15', '0.2', '0.25', '0.3', '0.35', '0.4']],
    ['lost_th', 'Lost Decision Count (windows)',
        ['1', '2', '3', '4', '5', '6']],
    ['bump_jump', 'Bump Jump for BOC Modulation (0/1)', ['0', '1']],
    ['max_acq', 'Max Code Length for Direct Acq (ms)',
        ['1.0', '2.0', '5.0', '10.0', '20.0']]
];

const N_PVT = 3; // number of the PVT options at the head of OPTS

// option value table (bare: also wrapped in an option group frame) ------------
function optbl(opts, bare) {
    const tbl = `<table class="optbl"><tbody>` +
        opts.map(([key, label, sels]) =>
            `<tr><td class="lbl">${label}</td>` +
            `<td><input type="number" step="any" id="op-${key}" ` +
            `list="op-dl-${key}"><datalist id="op-dl-${key}">` +
            sels.map(v => `<option value="${v}">`).join('') +
            `</datalist></td></tr>`).join('') + `</tbody></table>`;
    return bare ? tbl : `<div class="cfg-frm">${tbl}</div>`;
}

export class OptsPage {
    constructor(app) {
        this.app = app;
        this.el = document.createElement('div');
        this.el.innerHTML =
            `<div class="toolbar">` +
            `<label class="ttl">System Options</label>` +
            `<span class="space"></span>` +
            `<button id="op-def">Set Default</button>` +
            `<button id="op-apply">Apply</button>` +
            `<button id="op-reload">Reload</button>` +
            `</div>` +
            `<div class="op-body">` +
            `<div class="warn-txt cfg-warn" id="op-note"></div>` +
            optbl(OPTS.slice(0, N_PVT)) + // PVT options, as the Tk panels
            `<div class="cfg-frm">` + optbl(OPTS.slice(N_PVT), 1) +
            `<div class="cfg-row"><label>Signal Acquisition Mode</label>` +
            `<select id="op-acq"><option>Full</option><option>Fast</option>` +
            `</select></div></div>` +
            `<div class="cfg-row"><label>FFTW Wisdom Path</label>` +
            `<input type="text" id="op-fftw" class="wide"></div>` +
            `<div class="cfg-row"><label>Receiver Options</label>` +
            `<input type="text" id="op-opt" class="wide"></div>` +
            `<div class="cfg-note">Receiver Options: -ARCH=n -GAIN=dB ` +
            `-BW=MHz (applied at receiver start)</div></div>`;
        this.el.querySelector('#op-reload').onclick = () => {
            this.app.ws.get('opts');
        };
        this.el.querySelector('#op-def').onclick = () => {
            this.app.ws.send({cmd: 'opts_default'});
            this.app.msg('System options set to default.');
            this.app.ws.get('opts');
        };
        this.el.querySelector('#op-apply').onclick = () => { // all at once
            for (const [key] of OPTS) {
                const val = parseFloat(
                    this.el.querySelector('#op-' + key).value);
                if (isNaN(val)) continue;
                this.app.ws.send({cmd: 'setopt', name: key, value: val});
            }
            this.app.ws.send({cmd: 'set_sys',
                fftw: this.el.querySelector('#op-fftw').value,
                opt: this.el.querySelector('#op-opt').value,
                fast_acq: this.el.querySelector('#op-acq').value == 'Fast' ?
                    1 : 0});
            this.app.msg('System options applied.');
            this.app.ws.get('opts');
        };
        app.ws.on('opts', (msg) => this.update(msg));
        for (const ev of ['open', 'hello']) { // follow the run state
            app.ws.on(ev, () => {
                if (this.active) this.app.ws.get('opts');
            });
        }
    }
    update(msg) {
        if (!this.active) return;
        for (const [key] of OPTS) {
            const inp = this.el.querySelector('#op-' + key);
            if (msg[key] !== undefined && document.activeElement != inp) {
                inp.value = msg[key];
            }
        }
        for (const [id, val] of [['#op-fftw', msg.fftw], ['#op-opt', msg.opt]]) {
            const inp = this.el.querySelector(id);
            if (val !== undefined && document.activeElement != inp) {
                inp.value = val;
            }
        }
        if (msg.fast_acq !== undefined) {
            this.el.querySelector('#op-acq').value =
                msg.fast_acq ? 'Fast' : 'Full';
        }
        const dis = !msg.ena || msg.run; // options apply at channel creation
        for (const e of this.el.querySelectorAll('.op-body input, ' +
            '.op-body select, #op-apply, #op-def')) {
            e.disabled = dis;
        }
        this.el.querySelector('#op-note').textContent = !msg.ena ? '' :
            msg.run ? 'Stop the receiver to edit' : '';
    }
    show() {
        this.active = true;
        this.app.ws.get('opts');
    }
    hide() {
        this.active = false;
    }
}
