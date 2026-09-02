// Pocket SDR Web UI - Help page

const URL = 'https://github.com/tomojitakasu/PocketSDR';
const TITLE = 'An Open Source GNSS SDR (Software Defined Receiver)';
const COPYRIGHT = 'Copyright (C) 2021-2026 T.Takasu. All rights reserved.';

const PAGES = [ // page, what it shows
    ['Receiver', 'Receiver status, sky plot and signal C/N0 of the ' +
        'tracked satellites'],
    ['RF CH', 'Power spectral density and sample histograms of the RF ' +
        'channels, or the signal band assignments of all channels'],
    ['BB CH', 'Status of the baseband receiver channels'],
    ['Correlator', 'Correlator outputs, IP-QP and their history of a ' +
        'selected channel'],
    ['Satellites', 'Satellite status and C/N0 of all signals'],
    ['Solution', 'PVT solution as E/N/U time series or a horizontal track'],
    ['Array', 'Antenna array calibration and beam directions'],
    ['Log', 'Receiver log messages']
];
const CMDS = [ // command bar button, action
    ['Start', 'Start the receiver with the current options'],
    ['Stop', 'Stop the receiver. The options are saved to the settings file'],
    ['Input', 'Input source, IF data format and RF channel settings'],
    ['Output', 'Output streams, receiver log types and path keywords'],
    ['Signal', 'Satellites and signals to be tracked, RF CH assignments'],
    ['System', 'Receiver system options, FFTW wisdom and receiver options']
];

export class HelpPage {
    constructor(app) {
        this.app = app;
        this.el = document.createElement('div');
        this.el.innerHTML =
            `<div class="toolbar"><label class="ttl">Help</label>` +
            `<span class="space"></span>` +
            `<span class="mono" id="hp-ver"></span></div>` +
            `<div class="cfg-body hp-body">` +
            `<div class="hp-head">` +
            `<img src="img/favicon.svg" alt="" class="hp-logo">` +
            `<div><a href="${URL}" target="_blank" class="hp-name" ` +
            `id="hp-title">Pocket SDR</a>` +
            `<div class="hp-sub">${TITLE}</div></div></div>` +
            `<div class="hp-sec">Pages</div>` +
            `<table class="hp-tbl"><tbody>` +
            PAGES.map(([n, d]) => `<tr><td class="hp-k">${n}</td>` +
                `<td>${d}</td></tr>`).join('') +
            `</tbody></table>` +
            `<div class="hp-sec">Command Bar</div>` +
            `<table class="hp-tbl"><tbody>` +
            CMDS.map(([n, d]) => `<tr><td class="hp-k">${n}</td>` +
                `<td>${d}</td></tr>`).join('') +
            `</tbody></table>` +
            `<div class="hp-sec">Notes</div>` +
            `<ul class="hp-list">` +
            `<li>A left or right swipe on the page area switches to the ` +
            `next or previous page of the tab bar. Over a table wider than ` +
            `the screen, a short drag scrolls the table and a long swipe ` +
            `switches the page.</li>` +
            `<li>The options can be edited only while the receiver is ` +
            `stopped. They are restored at the next startup when the AP is ` +
            `started without the -sig option.</li>` +
            `<li>The Correlator page enables the additional correlators of ` +
            `the selected channel. They are disabled when no client shows ` +
            `the page.</li>` +
            `<li>The interface is not authenticated. Bind it to a trusted ` +
            `network only.</li>` +
            `</ul>` +
            `<div class="hp-sec">Documents</div>` +
            `<ul class="hp-list">` +
            `<li><a href="${URL}" target="_blank">PocketSDR on GitHub</a>` +
            `</li>` +
            `<li><a href="${URL}/blob/master/doc/command_ref.md" ` +
            `target="_blank">Command Reference</a></li>` +
            `<li><a href="${URL}/blob/master/doc/design_web_ui.md" ` +
            `target="_blank">Web UI Design Note</a></li>` +
            `</ul>` +
            `<div class="hp-copy">${COPYRIGHT}</div>` +
            `</div>`;
        app.ws.on('hello', (msg) => {
            this.el.querySelector('#hp-title').textContent = msg.name;
            this.el.querySelector('#hp-ver').textContent =
                msg.name + ' ver.' + msg.ver + ' / Web UI protocol ' +
                msg.proto;
        });
    }
    show() {
        this.active = true;
    }
    hide() {
        this.active = false;
    }
}
