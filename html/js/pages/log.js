// Pocket SDR Web UI - Log page

const FILTERS = ['', '$TIME', '$POS', '$ATT', '$OBS', '$NAV', '$SAT', '$CH',
    '$EPH', '$ALM', '$LOG', '$SENS'];
const MAX_LINES = 2000;

export class LogPage {
    constructor(app) {
        this.app = app;
        this.lines = [];
        this.paused = false;
        this.el = document.createElement('div');
        this.el.innerHTML =
            `<div class="toolbar">` +
            `<label>Filter</label><select id="lg-sel">` +
            FILTERS.map(f => `<option>${f}</option>`).join('') + `</select>` +
            `<input type="text" id="lg-txt" size="20">` +
            `<span class="space"></span>` +
            `<button id="lg-pause">Pause</button>` +
            `<button id="lg-clear">Clear</button>` +
            `</div>` +
            `<div class="log-body" id="lg-body"><pre id="lg-pre"></pre></div>`;
        this.body = this.el.querySelector('#lg-body');
        this.pre = this.el.querySelector('#lg-pre');
        // the filter applies to the lines received from now on, not to the
        // lines already shown, as the Tk GUI does
        this.el.querySelector('#lg-sel').onchange = () => this.setFilter();
        this.el.querySelector('#lg-txt').oninput = () => this.setFilter();
        this.el.querySelector('#lg-pause').onclick = () => {
            this.paused = !this.paused;
            this.el.querySelector('#lg-pause').textContent =
                this.paused ? 'Resume' : 'Pause';
            if (!this.paused) this.render();
        };
        this.el.querySelector('#lg-clear').onclick = () => {
            this.lines = [];
            this.render();
        };
        this.setFilter();
        app.ws.on('log', (msg) => {
            if (!this.active) return;
            this.lines.push(...msg.lines.filter(line => this.match(line)));
            if (this.lines.length > MAX_LINES) {
                this.lines.splice(0, this.lines.length - MAX_LINES);
            }
            if (!this.paused) this.render();
        });
    }
    setFilter() {
        const filt = (this.el.querySelector('#lg-sel').value + ' ' +
            this.el.querySelector('#lg-txt').value).trim();
        this.terms = filt ? filt.split(/\s+/).map(t => t.split('|')) : null;
    }
    match(line) {
        return !this.terms ||
            this.terms.every(alts => alts.some(a => line.includes(a)));
    }
    render() {
        this.pre.textContent = this.lines.join('\n');
        this.body.scrollTop = this.body.scrollHeight;
    }
    show() { // keep the lines already received; the server resumes from there
        this.active = true;
        this.render();
        this.app.ws.sub('log', {cyc: 200});
    }
    hide() {
        this.active = false;
        this.app.ws.unsub('log');
    }
}
