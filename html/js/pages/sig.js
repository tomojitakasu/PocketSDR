// Pocket SDR Web UI - Signal Options page

const SYS_SIGS = { // selectable signals by satellite system
    GPS: ['L1CA', 'L1CD', 'L1CP', 'L2CM', 'L5I', 'L5Q'],
    GLONASS: ['G1CA', 'G1OCD', 'G1OCP', 'G2CA', 'G2OCP', 'G3OCD', 'G3OCP'],
    Galileo: ['E1B', 'E1C', 'E5AI', 'E5AQ', 'E5ABQ', 'E5BI', 'E5BQ', 'E6B',
        'E6C'],
    QZSS: ['L1CA', 'L1CB', 'L1CD', 'L1CP', 'L1S', 'L2CM', 'L5I', 'L5Q',
        'L5SI', 'L5SIV', 'L5SQ', 'L5SQV', 'L6D', 'L6E'],
    BeiDou: ['B1I', 'B1CD', 'B1CP', 'B2AD', 'B2AP', 'B2I', 'B2BI', 'B3I'],
    NavIC: ['I1SD', 'I1SP', 'I5S', 'ISS'],
    SBAS: ['L1CA', 'L5I', 'L5Q']
};
const DEF_SAT = { // default satellite number lists (GLONASS: FCNs/slots)
    GPS: '1-32', GLONASS: '-7-6/1-27', Galileo: '1-36', QZSS: '1-9',
    BeiDou: '1-63', NavIC: '1-14', SBAS: '120-158'
};
// systems in the order of the satellite number lists of the configuration
const SYSTEMS = Object.keys(SYS_SIGS);

const SYS_CODE = { // system codes for the satellite No assignments
    GPS: 'G', GLONASS: 'R', Galileo: 'E', QZSS: 'J', BeiDou: 'C', NavIC: 'I',
    SBAS: 'S'
};

// parse number list with ranges (e.g. "1-32,40", "-7-6") ----------------------
function parseNums(str) {
    const nums = [];
    for (const tok of str.split(',')) {
        const m = tok.trim().match(/^(-?\d+)(?:-(-?\d+))?$/);
        if (!m) continue;
        const a = parseInt(m[1]);
        const b = m[2] !== undefined ? parseInt(m[2]) : a;
        for (let n = a; n <= b && n - a < 512; n++) nums.push(n);
    }
    return nums;
}

// compress number list to ranges ----------------------------------------------
function compNums(nums) {
    const s = [...new Set(nums)].sort((a, b) => a - b);
    const out = [];
    for (let i = 0; i < s.length; ) {
        let j = i;
        while (j + 1 < s.length && s[j+1] == s[j] + 1) j++;
        out.push(j > i ? s[i] + '-' + s[j] : '' + s[i]);
        i = j + 1;
    }
    return out.join(',');
}

// QZSS satellite number to PRN (as python qzss_no2prn) ------------------------
function qzssNo2Prn(sig, no) {
    if (['L1CA', 'L1CD', 'L1CP', 'L2CM', 'L5I', 'L5Q', 'L6D'].includes(sig)) {
        return 192 + no;
    }
    if (sig == 'L1S' && no <= 7) return 182 + no;
    if (sig == 'L6E') return 202 + no;
    if (sig == 'L1CB' && [4, 5, 8, 9, 10].includes(no)) {
        return no <= 5 ? 199 + no : no <= 9 ? 197 + no : 202;
    }
    if (['L5SI', 'L5SQ'].includes(sig) && [2, 3, 4, 7].includes(no)) {
        return 182 + no;
    }
    if (['L5SIV', 'L5SQV'].includes(sig) && [4, 8, 9].includes(no)) {
        return no == 4 ? 186 : 197 + no;
    }
    return 0;
}

// satellite number list to PRN list for a signal (null: none) -----------------
function sat2prns(sys, sig, satno) {
    if (sys == 'QZSS') {
        const prns = parseNums(satno).map(n => qzssNo2Prn(sig, n))
            .filter(p => p > 0);
        return prns.length ? compNums(prns) : null;
    }
    if (sys == 'GLONASS') { // "fcns/slots"
        const s = satno.split('/');
        const part = sig == 'G1CA' || sig == 'G2CA' ? s[0] : s[1] || '';
        return part.trim() ? part.trim() : null;
    }
    return satno;
}

// parse satellite No assignments "[<sys>:]<sig>:<prns> ..." ------------------
function parseSatAsgn(str) {
    const asgn = {};
    for (const tok of (str || '').trim().split(/\s+/)) {
        const p = tok.split(':');
        // 2 fields: all systems having the signal, 3 fields: one system
        if (p.length == 2) asgn['*:' + p[0].toUpperCase()] = p[1];
        else if (p.length == 3) {
            asgn[p[0].toUpperCase() + ':' + p[1].toUpperCase()] = p[2];
        }
    }
    return asgn;
}

// PRN list of an assignment for a signal (undefined: not assigned) ------------
function asgnPrns(asgn, sys, sig) {
    const s = asgn[SYS_CODE[sys] + ':' + sig];
    if (s !== undefined) return s;
    const t = asgn['*:' + sig];
    if (t === undefined || !t.trim()) return t;
    // a system-less assignment goes to the systems its PRNs belong to
    const prns = parseNums(t).filter(p => sigSys(sig, '' + p) == sys);
    return prns.length ? compNums(prns) : '';
}

// system of a "SIG:prns" entry (GPS, QZSS and SBAS share signal names) --------
function sigSys(sig, prns) {
    const cand = Object.keys(SYS_SIGS).filter(s => SYS_SIGS[s].includes(sig));
    if (cand.length < 2) return cand[0] || '';
    const prn = parseNums(prns)[0] || 0; // GPS 1-32, SBAS 120-158, QZSS 183-
    const sys = prn >= 183 ? 'QZSS' : prn >= 120 ? 'SBAS' : 'GPS';
    return cand.includes(sys) ? sys : cand[0];
}

export class SigPage {
    constructor(app) {
        this.app = app;
        this.el = document.createElement('div');
        this.el.innerHTML =
            `<div class="toolbar">` +
            `<label class="ttl">Signal Options</label>` +
            `<span class="space"></span>` +
            `<button id="sg-all">Set All</button>` +
            `<button id="sg-none">Unset All</button>` +
            `<button id="sg-apply">Apply</button>` +
            `<button id="sg-reload">Reload</button>` +
            `</div>` +
            `<div class="cfg-body">` +
            `<div class="warn-txt cfg-warn" id="sg-note"></div>` +
            `<div class="sig-sec sig-hdr">` +
            `<span class="sig-head">System</span>` +
            `<span class="sig-sat">Satellite No</span>` +
            `<span class="sig-sigs">GNSS Signals</span></div>` +
            Object.keys(SYS_SIGS).map(sys =>
                `<div class="sig-sec" data-sys="${sys}">` +
                `<label class="sig-head"><input type="checkbox" ` +
                `class="sg-sys" checked> ${sys}</label>` +
                `<input type="text" class="sg-prn sig-sat" ` +
                `value="${DEF_SAT[sys]}">` +
                `<span class="sig-grid">` +
                SYS_SIGS[sys].map(sig =>
                    `<label class="sig-chk"><input type="checkbox" ` +
                    `class="sg-sig" data-sig="${sig}"> ${sig}</label>`
                ).join('') + `</span></div>`).join('') +
            `<div class="cfg-frm">` +
            `<div class="cfg-row"><label>Satellite No Assignments</label>` +
            `<input type="text" id="sg-satno" class="wide"></div>` +
            `<div class="cfg-row"><label>RF CH Assignments</label>` +
            `<input type="text" id="sg-rfch" class="wide"></div>` +
            `<div class="cfg-note">Satellite No Assignments: ` +
            `[&lt;sys&gt;:]&lt;sig&gt;:&lt;prn&gt;[{,|-}&lt;prn&gt;...] ... ` +
            `(&lt;sys&gt;: G,R,E,J,C,I,S, empty list: no CH, ` +
            `overrides Satellite No) / RF CH Assignments: ` +
            `&lt;sig&gt;:&lt;ch&gt;[{,|-}&lt;ch&gt;...] ... (empty: auto) / ` +
            `GLONASS SAT: fcns/slots</div></div>` +
            `</div>`;
        this.el.querySelector('#sg-apply').onclick = () => this.apply();
        this.el.querySelector('#sg-reload').onclick = () =>
            this.app.ws.get('cfg');
        this.el.querySelector('#sg-all').onclick = () => this.setAll(true);
        this.el.querySelector('#sg-none').onclick = () => this.setAll(false);
        for (const e of this.el.querySelectorAll('.sg-sys')) {
            e.onchange = () => this.updateEna();
        }
        app.ws.on('cfg', (msg) => {
            if (this.active) this.populate(msg);
        });
        app.ws.on('hello', () => {
            if (this.active) this.app.ws.get('cfg');
        });
    }
    // satellite numbers and signals follow the system enable check ------------
    updateEna() {
        const dis = this.el.querySelector('#sg-apply').disabled;
        for (const sec of this.el.querySelectorAll('.sig-sec[data-sys]')) {
            const ena = sec.querySelector('.sg-sys').checked;
            for (const e of sec.querySelectorAll('.sg-prn, .sg-sig')) {
                e.disabled = dis || !ena;
            }
        }
    }
    setEditable(ena, run) {
        const dis = !ena || run;
        for (const id of ['#sg-apply', '#sg-all', '#sg-none']) {
            this.el.querySelector(id).disabled = dis;
        }
        for (const e of this.el.querySelectorAll('.cfg-body input')) {
            e.disabled = dis;
        }
        this.el.querySelector('#sg-note').textContent = !ena ?
            'Configuration not supported by the server' :
            run ? 'Stop the receiver to edit' : '';
        this.updateEna();
    }
    setAll(ena) {
        for (const e of this.el.querySelectorAll('.sg-sig')) e.checked = ena;
        for (const e of this.el.querySelectorAll('.sg-sys')) e.checked = ena;
        this.updateEna();
    }
    satEl(sys) {
        return this.el.querySelector(`.sig-sec[data-sys=${sys}] .sg-prn`);
    }
    populate(cfg) {
        this.el.querySelector('#sg-rfch').value = cfg.rfch;
        this.el.querySelector('#sg-satno').value = cfg.satasgn || '';
        const satno = (cfg.satno || '').split('|');
        SYSTEMS.forEach((sys, i) => { // the satellite numbers are a setting
            if (satno[i]) this.satEl(sys).value = satno[i];
        });
        for (const sec of this.el.querySelectorAll('.sig-sec[data-sys]')) {
            sec.querySelector('.sg-sys').checked = false;
            for (const e of sec.querySelectorAll('.sg-sig')) e.checked = false;
        }
        // assign "SIG:prns" entries to their systems (the PRN tells them apart)
        for (const ent of cfg.sigs.split(/\s+/)) {
            const [sig, prn] = ent.split(':');
            if (!sig || !prn) continue;
            const sys = sigSys(sig, prn);
            const sec = sys &&
                this.el.querySelector(`.sig-sec[data-sys=${sys}]`);
            if (!sec) continue;
            [...sec.querySelectorAll('.sg-sig')].find(
                e => e.dataset.sig == sig).checked = true;
            sec.querySelector('.sg-sys').checked = true;
        }
        this.setEditable(cfg.ena, cfg.run);
    }
    apply() {
        const sigs = [];
        const asgn = parseSatAsgn(this.el.querySelector('#sg-satno').value);
        const secs = [...this.el.querySelectorAll('.sig-sec[data-sys]')];
        const nsig = Math.max(...secs.map(
            s => s.querySelectorAll('.sg-sig').length));
        // signal index first, then system, as the Tk GUI assigns receiver CHs
        for (let i = 0; i < nsig; i++) {
            for (const sec of secs) {
                if (!sec.querySelector('.sg-sys').checked) continue;
                const e = sec.querySelectorAll('.sg-sig')[i];
                if (!e || !e.checked) continue;
                const sys = sec.dataset.sys;
                const satno = sec.querySelector('.sg-prn').value.trim() ||
                    DEF_SAT[sys];
                // an assignment overrides the satellite numbers of the system
                let prns = asgnPrns(asgn, sys, e.dataset.sig);
                if (prns === undefined) {
                    prns = sat2prns(sys, e.dataset.sig, satno);
                }
                if (prns && prns.trim()) {
                    sigs.push(e.dataset.sig + ':' + prns.trim());
                }
            }
        }
        this.app.ws.send({cmd: 'set_sig', sigs: sigs.join(' '),
            satno: SYSTEMS.map(
                sys => this.satEl(sys).value.trim() || DEF_SAT[sys]).join('|'),
            satasgn: this.el.querySelector('#sg-satno').value.trim(),
            rfch: this.el.querySelector('#sg-rfch').value});
        this.app.msg('Signal options applied.');
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
