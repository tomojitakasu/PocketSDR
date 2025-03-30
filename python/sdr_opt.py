#
#  Pocket SDR Python Library - GNSS SDR Receiver Option Functions
#
#  Author:
#  T.TAKASU
#
#  History:
#  2024-06-10  1.0  new
#  2024-12-30  1.1  update input options, output options, system options
#
import os, re, webbrowser
from tkinter import *
from tkinter import ttk
from tkinter import filedialog

# constants --------------------------------------------------------------------
MAX_RFCH = 8           # max number of RF channels
BG_COLOR = '#F8F8F8'   # background color
DLG_MARGIN = (20, 15)  # dialog margin
BAND_LINK = 'file://' + os.path.dirname(__file__) + '/../doc/signal_bands.pdf'
SIG_LINK = 'file://' + os.path.dirname(__file__) + '/../doc/signal_IDs.pdf'
FONT = ('Tahoma', 9, 'normal')

# general object class ---------------------------------------------------------
class Obj: pass

# set background color ---------------------------------------------------------
def set_bgcolor(color):
    global BG_COLOR
    BG_COLOR = color

# set font ---------------------------------------------------------------------
def set_font(font):
    global FONT
    FONT = font

# generate modal dialog --------------------------------------------------------
def modal_dlg_new(root, width, height, title='', nocancel=0):
    dlg = Obj()
    dlg.ok = 0
    dlg.win = Toplevel(bg=BG_COLOR)
    x = root.winfo_x() + (root.winfo_width()  - width ) / 2
    y = root.winfo_y() + (root.winfo_height() - height) / 2
    dlg.win.geometry('%dx%d+%d+%d' % (width, height, x, y)) # place root center
    dlg.win.resizable(width=False, height=False)
    dlg.win.title(title)
    dlg.panel = Frame(dlg.win, bg=BG_COLOR)
    dlg.panel.pack(fill=BOTH, expand=1, padx=DLG_MARGIN[0], pady=DLG_MARGIN[1])
    btns = Frame(dlg.panel, bg=BG_COLOR)
    btns.pack(side=BOTTOM, fill=X)
    if not nocancel:
        btn2 = ttk.Button(btns, width=14, padding=(2, 2), text='Cancel')
        btn2.bind('<Button-1>', lambda e: on_modal_dlg_cancel(e, dlg))
        btn2.pack(side=RIGHT, padx=1)
    btn1 = ttk.Button(btns, width=14, padding=(2, 2), text='OK')
    btn1.bind('<Button-1>', lambda e: on_modal_dlg_ok(e, dlg))
    btn1.pack(side=TOP if nocancel else RIGHT, padx=1)
    dlg.win.update()
    dlg.win.focus_set()
    dlg.win.grab_set()
    return dlg

# modal dialog OK callback -----------------------------------------------------
def on_modal_dlg_ok(e, dlg):
    dlg.ok = 1
    dlg.win.destroy()

# modal dialog cancel callback -------------------------------------------------
def on_modal_dlg_cancel(e, dlg):
    dlg.win.destroy()

# generate custom button -------------------------------------------------------
def custom_btn_new(parent, label='', font=FONT):
    p = Obj()
    p.panel = Frame(parent, bg='#AAAAAA')
    p.btn = ttk.Label(p.panel, text=label, anchor=CENTER, background='white')
    p.btn.pack(expand=1, fill=BOTH, padx=1, pady=1)
    p.btn.bind('<Enter>', lambda e: e.widget.configure(background = '#DDDDDD'))
    p.btn.bind('<Leave>', lambda e: e.widget.configure(background = 'white'))
    return p

# generate linked label --------------------------------------------------------
def link_label_new(parent, text='', link='', font=FONT):
    label = ttk.Label(parent, text=text, foreground='blue', font=font)
    label.bind('<Enter>', lambda e: e.widget.configure(foreground = 'red'))
    label.bind('<Leave>', lambda e: e.widget.configure(foreground = 'blue'))
    label.bind('<Button-1>', lambda e: webbrowser.open(link))
    return label

# generate labels panel --------------------------------------------------------
def labels_panel(parent, labels, xs):
    panel = Frame(parent, height=20, bg=BG_COLOR)
    for i in range(len(labels)):
        ttk.Label(panel, text=labels[i]).place(x=xs[i])
    return panel

# generate option selection panel ----------------------------------------------
def sel_panel_new(parent, label, sels=[], var=None, width=9):
    panel = Frame(parent, bg=BG_COLOR)
    ttk.Label(panel, text=label).pack(side=LEFT, fill=X)
    ttk.Combobox(panel, width=width, values=sels, textvariable=var,
        justify=CENTER, font=FONT).pack(side=RIGHT)
    panel.pack(fill=X, padx=(10, 4), pady=(3, 1))
    return panel

# generate option input panel --------------------------------------------------
def inp_panel_new(parent, label, var=None, width=10, pwidth=300, justify=RIGHT):
    panel = Frame(parent, width=pwidth, bg=BG_COLOR)
    panel.pack()
    ttk.Label(panel, text=label).pack(side=LEFT, fill=X, padx=2)
    ttk.Entry(panel, width=width, justify=justify, textvariable=var,
        font=FONT).pack(side=RIGHT, padx=2)
    panel.pack(fill=X, padx=(8, 4), pady=2)
    return panel

# generate option path panel ---------------------------------------------------
def path_panel_new(parent, label, out=0, var_path=None, var_ena=None, types=[]):
    panel = Frame(parent, bg=BG_COLOR)
    if var_ena != None:
        btn1 = ttk.Checkbutton(panel, text=label, variable=var_ena).pack(fill=X,
            pady=2)
    else:
        ttk.Label(panel, text=label).pack(fill=X, pady=2)
    panel1 = Frame(panel, bg=BG_COLOR)
    panel1.pack(fill=X, pady=(0, 2))
    p = custom_btn_new(panel1, ' ... ')
    p.panel.pack(side=RIGHT, padx=2, pady=1)
    inp = ttk.Entry(panel1, textvariable=var_path, font=FONT)
    inp.pack(side=LEFT, expand=1, fill=X)
    p.btn.bind('<Button-1>', lambda e: on_path_btn_push(e, panel, var_path, out,
        types))
    panel.pack(fill=X, padx=(10, 2), pady=2)
    return panel

# option path button push callback ---------------------------------------------
def on_path_btn_push(e, p, var, out=0, types=()):
    if out:
        path = filedialog.asksaveasfilename(parent=p, title='Output File Path',
            filetypes=types)
    else:
        path = filedialog.askopenfilename(parent=p, title='Input File Path',
            filetypes=types, initialfile=var.get())
    if path != '':
        var.set(path)

# set state of all widgets in panel --------------------------------------------
def config_panel_state(p, state):
    for c in p.winfo_children():
        if c.winfo_class() == 'Frame':
            config_panel_state(c, state)
        else:
            c.configure(state=state)

# generate input option variables ---------------------------------------------
def inp_opt_new(opt_p=None):
    fo_def = ['1568.000', '1227.600', '1176.450', '1278.750', '1602.000',
        '1246.000', '1207.140', '1268.520']
    opt = Obj()
    opt.inps = ('RF Frontend', 'IF Data')
    opt.types = ('Pocket SDR FE',)
    opt.fmts = ('INT8', 'INT8X2', 'RAW8', 'RAW16', 'RAW32')
    opt.IQs = ('IQ', 'I')
    opt.bitss = ('2', '3')
    opt.inp = IntVar()
    opt.type = StringVar()
    opt.dev = StringVar()
    opt.conf_ena = IntVar()
    opt.conf_path = StringVar()
    opt.fmt = StringVar()
    opt.fo = [StringVar() for i in range(MAX_RFCH)]
    opt.IQ = [StringVar() for i in range(MAX_RFCH)]
    opt.bits = [StringVar() for i in range(MAX_RFCH)]
    opt.fs = StringVar()
    opt.str_path = StringVar()
    opt.toff = StringVar()
    opt.tscale = StringVar()
    if opt_p != None:
        opt.inp.set(opt_p.inp.get())
        opt.type.set(opt_p.type.get())
        opt.dev.set(opt_p.dev.get())
        opt.conf_ena.set(opt_p.conf_ena.get())
        opt.conf_path.set(opt_p.conf_path.get())
        opt.fmt.set(opt_p.fmt.get())
        opt.fs.set(opt_p.fs.get())
        for i in range(len(opt.fo)):
            opt.fo[i].set(opt_p.fo[i].get())
            opt.IQ[i].set(opt_p.IQ[i].get())
            opt.bits[i].set(opt_p.bits[i].get())
        opt.str_path.set(opt_p.str_path.get())
        opt.toff.set(opt_p.toff.get())
        opt.tscale.set(opt_p.tscale.get())
    else:
        opt.type.set(opt.types[0])
        opt.fmt.set(opt.fmts[0])
        opt.fs.set('24.000')
        for i in range(len(opt.fo)):
            opt.fo[i].set(fo_def[i])
            opt.IQ[i].set(opt.IQs[0])
            opt.bits[i].set(opt.bitss[0])
        opt.toff.set('0.0')
        opt.tscale.set('1.0')
    return opt

# generate output option variables ---------------------------------------------
def out_opt_new(opt_p=None):
    opt = Obj()
    opt.log = ('TIME', 'POS', 'OBS', 'NAV', 'SAT', 'CH', 'EPH', 'LOG')
    opt.path_ena = [IntVar() for i in range(4)]
    opt.path = [StringVar() for i in range(4)]
    opt.log_sel = [IntVar() for s in opt.log]
    if opt_p != None:
        for i in range(4):
            opt.path_ena[i].set(opt_p.path_ena[i].get())
            opt.path[i].set(opt_p.path[i].get())
        for i in range(len(opt.log_sel)):
            opt.log_sel[i].set(opt_p.log_sel[i].get())
    else:
        for i in range(len(opt.log_sel)):
            opt.log_sel[i].set(0 if i in (5, 6) else 1)
    return opt

# generate signal option variables ---------------------------------------------
def sig_opt_new(opt_p=None):
    satno_def = ('1-32', '-7-6/1-27', '1-36', '1-9', '1-63', '1-14', '120-158')
    opt = Obj()
    opt.sys = ('GPS', 'GLONASS', 'Galileo', 'QZSS', 'BeiDou', 'NavIC', 'SBAS')
    opt.sig = (
        ('L1CA', 'L1CD', 'L1CP', 'L2CM', 'L5I', 'L5Q'),
        ('G1CA', 'G1OCD', 'G1OCP', 'G2CA', 'G2OCP', 'G3OCD', 'G3OCP'),
        ('E1B', 'E1C', 'E5AI', 'E5AQ', 'E5BI', 'E5BQ', 'E6B', 'E6C'),
        ('L1CA', 'L1CB', 'L1CD', 'L1CP', 'L1S', 'L2CM', 'L5I', 'L5Q', 'L5SI',
         'L5SIV', 'L5SQ', 'L5SQV', 'L6D', 'L6E'),
        ('B1I', 'B1CD', 'B1CP', 'B2AD', 'B2AP', 'B2I', 'B2BI', 'B3I'),
        ('I1SD', 'I1SP', 'I5S', 'ISS'), ('L1CA', 'L5I', 'L5Q'))
    opt.sys_sel = [IntVar() for s in opt.sys]
    opt.satno = [StringVar() for s in opt.sys]
    opt.sig_sel = [[IntVar() for s in opt.sig[i]] for i in range(len(opt.sig))]
    opt.sig_rfch = StringVar()
    if opt_p != None:
        for i in range(len(opt.sys_sel)):
            opt.sys_sel[i].set(opt_p.sys_sel[i].get())
            opt.satno[i].set(opt_p.satno[i].get())
            for j in range(len(opt.sig_sel[i])):
                opt.sig_sel[i][j].set(opt_p.sig_sel[i][j].get())
        opt.sig_rfch.set(opt_p.sig_rfch.get())
    else:
        for i in range(len(opt.satno)):
            opt.satno[i].set(satno_def[i])
    return opt

# generate system option variables ---------------------------------------------
def sys_opt_new(opt_p=None):
    opt = Obj()
    opt.epoch = StringVar()
    opt.lag_epoch = StringVar()
    opt.el_mask = StringVar()
    opt.sp_corr = StringVar()
    opt.t_acq = StringVar()
    opt.t_dll = StringVar()
    opt.b_dll = StringVar()
    opt.b_pll = StringVar()
    opt.b_fll_w = StringVar()
    opt.b_fll_n = StringVar()
    opt.max_dop = StringVar()
    opt.thres_cn0_l = StringVar()
    opt.thres_cn0_u = StringVar()
    opt.bump_jump = StringVar()
    opt.fftw_wisdom_path = StringVar()
    if opt_p != None:
        opt.epoch.set(opt_p.epoch.get())
        opt.lag_epoch.set(opt_p.lag_epoch.get())
        opt.el_mask.set(opt_p.el_mask.get())
        opt.sp_corr.set(opt_p.sp_corr.get())
        opt.t_acq.set(opt_p.t_acq.get())
        opt.t_dll.set(opt_p.t_dll.get())
        opt.b_dll.set(opt_p.b_dll.get())
        opt.b_pll.set(opt_p.b_pll.get())
        opt.b_fll_w.set(opt_p.b_fll_w.get())
        opt.b_fll_n.set(opt_p.b_fll_n.get())
        opt.max_dop.set(opt_p.max_dop.get())
        opt.thres_cn0_l.set(opt_p.thres_cn0_l.get())
        opt.thres_cn0_u.set(opt_p.thres_cn0_u.get())
        opt.bump_jump.set(opt_p.bump_jump.get())
        opt.fftw_wisdom_path.set(opt_p.fftw_wisdom_path.get())
    else:
        opt.epoch.set('1.0')
        opt.lag_epoch.set('0.5')
        opt.el_mask.set('15')
        opt.sp_corr.set('0.25')
        opt.t_acq.set('0.02')
        opt.t_dll.set('0.02')
        opt.b_dll.set('0.25')
        opt.b_pll.set('5.0')
        opt.b_fll_w.set('5.0')
        opt.b_fll_n.set('2.0')
        opt.max_dop.set('5000')
        opt.thres_cn0_l.set('34.0')
        opt.thres_cn0_u.set('30.0')
        opt.bump_jump.set('OFF')
    return opt

# save options -----------------------------------------------------------------
def save_opts(file, inp_opt, out_opt, sig_opt, sys_opt):
    try:
        f = open(file, 'w')
        save_opt('inp_opt', inp_opt, f)
        save_opt('out_opt', out_opt, f)
        save_opt('sig_opt', sig_opt, f)
        save_opt('sys_opt', sys_opt, f)
        f.close()
    except:
        print('options save error: %s' % (file))

# save option ------------------------------------------------------------------
def save_opt(opt_name, opt, f):
    f.write('[%s]\n' % (opt_name))
    for key, val in vars(opt).items():
        write_opt(f, key, val)

# write option -----------------------------------------------------------------
def write_opt(f, key, val):
    class_name = val.__class__.__name__
    if class_name == 'IntVar':
        f.write('%s=%d\n' % (key, val.get()))
    elif class_name == 'StringVar':
        f.write('%s=%s\n' % (key, val.get()))
    elif class_name == 'list':
        for i in range(len(val)):
            write_opt(f, key + '@%d' % (i), val[i])

# load options -----------------------------------------------------------------
def load_opts(file, inp_opt, out_opt, sig_opt, sys_opt):
    try:
        f = open(file, 'r')
        txt = f.readlines()
        load_opt('inp_opt', inp_opt, txt)
        load_opt('out_opt', out_opt, txt)
        load_opt('sig_opt', sig_opt, txt)
        load_opt('sys_opt', sys_opt, txt)
        f.close()
    except:
        print('options load error: %s' % (file))

# load option ------------------------------------------------------------------
def load_opt(opt_name, opt, txt):
    flag = 0
    for s in txt:
        if s[0] == '[':
            flag = s[:-1] == '[' + opt_name + ']'
            continue
        if not flag:
            continue
        ss = s[:-1].split('=', 1)
        sss = ss[0].split('@')
        for key, val in vars(opt).items():
            if key == sss[0]:
                if len(sss) == 1:
                    read_opt(val, ss[1])
                elif len(sss) == 2:
                    read_opt(val[int(sss[1])], ss[1])
                elif len(sss) == 3:
                    read_opt(val[int(sss[1])][int(sss[2])], ss[1])

# read option ------------------------------------------------------------------
def read_opt(val, opt):
    class_name = val.__class__.__name__
    if class_name == 'IntVar':
        val.set(int(opt))
    elif class_name == 'StringVar':
        val.set(opt)

# show Input Options dialog ----------------------------------------------------
def inp_opt_dlg(root, opt):
    opt_new = inp_opt_new(opt)
    dlg = modal_dlg_new(root, 480, 600, 'Input Options')
    panel = Frame(dlg.panel, width=450, bg=BG_COLOR)
    panel.pack(fill=X, pady=4)
    ttk.Label(panel, text='Input Source').pack(side=LEFT, padx=4)
    btn1 = ttk.Radiobutton(panel, text=opt.inps[0], var=opt_new.inp, value=0)
    btn2 = ttk.Radiobutton(panel, text=opt.inps[1], var=opt_new.inp, value=1)
    btn2.pack(side=RIGHT, padx=(15, 75))
    btn1.pack(side=RIGHT, padx=15)
    p1 = rf_opt_panel_new(dlg.panel, opt_new)
    p2 = if_opt_panel_new(dlg.panel, opt_new)
    p3 = ch_opt_panel_new(dlg.panel, opt_new)
    p4 = Frame(dlg.panel, bg=BG_COLOR)
    p4.pack(fill=X)
    text = '* Automatically configured if <Path>.tag file exists.'
    ttk.Label(p4, text=text, anchor=N).pack(fill=X)
    ch_opt_enable_update(opt_new.fmt.get(), p3)
    inp_opt_enable_update(opt_new.inp.get(), p1, p2, p3, p4)
    btn1.bind('<Button-1>', lambda e: on_inp_select(e, 0, p1, p2, p3, p4))
    btn2.bind('<Button-1>', lambda e: on_inp_select(e, 1, p1, p2, p3, p4))
    root.wait_window(dlg.win)
    return opt_new if dlg.ok else opt

# input source select callback -------------------------------------------------
def on_inp_select(e, sel, p1, p2, p3, p4):
    inp_opt_enable_update(sel, p1, p2, p3, p4)

# update input options enable --------------------------------------------------
def inp_opt_enable_update(sel, p1, p2, p3, p4):
    config_panel_state(p1, DISABLED if sel else NORMAL)
    config_panel_state(p2, NORMAL if sel else DISABLED)
    config_panel_state(p3, NORMAL if sel else DISABLED)
    config_panel_state(p4, NORMAL if sel else DISABLED)
    if sel:
        fmt = p3.winfo_children()[0].winfo_children()[1].get()
        ch_opt_enable_update(fmt, p3)

# generate RF Frontend options panel -------------------------------------------
def rf_opt_panel_new(parent, opt):
    panel = Frame(parent, bg=BG_COLOR, relief=GROOVE, borderwidth=2)
    ttk.Label(panel, text=opt.inps[0], justify=LEFT).pack(fill=X, padx=2, pady=2)
    sel_panel_new(panel, 'Device Type', sels=opt.types, var=opt.type, width=19)
    panel1 = Frame(panel, bg=BG_COLOR)
    panel1.pack(fill=X)
    ttk.Label(panel1, text='Device Selection (Blank: Any)'). pack(side=LEFT,
        padx=(10, 4), pady=2)
    inp_panel_new(panel1, 'USB Bus/Port', opt.dev).pack(side=RIGHT, padx=(4, 2))
    path_panel_new(panel, 'Device Configuration File', var_path=opt.conf_path,
        var_ena=opt.conf_ena, types=[('Config File', '*.conf'), ('All', '*.*')])
    panel.pack(fill=X, pady=2)
    return panel

# generate IF Data options panel -----------------------------------------------
def if_opt_panel_new(parent, opt):
    panel = Frame(parent, width=300, height=300, bg=BG_COLOR, relief=GROOVE,
        borderwidth=2)
    ttk.Label(panel, text=opt.inps[1]).pack(fill=X, padx=2, pady=2)
    path_panel_new(panel, 'Path (File: local_path)',
        var_path=opt.str_path, types=[('Raw IF Data', '*.bin'), ('All', '*.*')])
    p1 = Frame(panel, bg=BG_COLOR)
    p1.pack(fill=X)
    inp_panel_new(p1, 'Time Offset (s)', opt.toff, width=10).pack(side=LEFT)
    inp_panel_new(p1, 'Time Scale', opt.tscale, width=10).pack(
        side=RIGHT, padx=(0, 28))
    panel.pack(fill=X, pady=2)
    return panel

# generate channel options panel -----------------------------------------------
def ch_opt_panel_new(parent, opt):
    panel = Frame(parent, height=300, bg=BG_COLOR, relief=GROOVE,
        borderwidth=2)
    panel.pack(fill=X, pady=2)
    p1 = sel_panel_new(panel, 'IF Data Format*', opt.fmts, opt.fmt)
    p1.pack(pady=(4, 2))
    p2 = p1.winfo_children()[1]
    p2.bind('<<ComboboxSelected>>', lambda e: on_fmt_select(e, panel))
    inp_panel_new(panel, 'Sampling Rate (Msps)*', opt.fs)
    panel1 = Frame(panel, bg=BG_COLOR)
    panel1.pack(fill=X, pady=(4, 0))
    label = ' RF   LO Freq (MHz)*  I/IQ*   Bits* '
    ttk.Label(panel1, text=label).pack(side=LEFT, padx=(10, 0))
    ttk.Label(panel1, text=label).pack(side=RIGHT, padx=(0, 5))
    panel2 = Frame(panel, bg=BG_COLOR)
    panel2.pack(side=LEFT, pady=2)
    panel3 = Frame(panel, bg=BG_COLOR)
    panel3.pack(side=RIGHT, pady=2)
    n = MAX_RFCH // 2
    for ch in range(0, n):
        rfch_opt_panel_new(panel2, ch, opt)
    for ch in range(n, MAX_RFCH):
        rfch_opt_panel_new(panel3, ch, opt)
    return panel

# IF data format select callback -----------------------------------------------
def on_fmt_select(e, p3):
    ch_opt_enable_update(e.widget.get(), p3)

# update channel options enable ------------------------------------------------
def ch_opt_enable_update(fmt, p3):
    p = p3.winfo_children()
    p2 = [p[3].winfo_children(), p[4].winfo_children()]
    n = MAX_RFCH // 2
    for i in range(MAX_RFCH):
        ena = (fmt in ('INT8', 'INT8X2') and i < 1) or \
            (fmt == 'RAW8' and i < 2) or (fmt == 'RAW16' and i < 4) or \
            (fmt == 'RAW32' and i < 8)
        config_panel_state(p2[i//n][i%n], NORMAL if ena else DISABLED)
    #p1 = p[1].winfo_children()[1]
    #p1.configure(stat=DISABLED if fmt in ('INT8', 'INT8X2') else NORMAL)

# generate RF channel options panel --------------------------------------------
def rfch_opt_panel_new(parent, ch, opt):
    panel = Frame(parent, bg=BG_COLOR)
    ttk.Label(panel, text='CH%d' % (ch + 1)).pack(side=LEFT, padx=(0, 8))
    ttk.Combobox(panel, width=2, justify=CENTER, values=opt.bitss,
        textvariable=opt.bits[ch], font=FONT).pack(side=RIGHT, padx=1)
    ttk.Combobox(panel, width=3, justify=CENTER, values=opt.IQs,
        textvariable=opt.IQ[ch], font=FONT).pack(side=RIGHT, padx=1)
    ttk.Entry(panel, width=10, justify=RIGHT, textvariable=opt.fo[ch],
        font=FONT).pack(side=RIGHT, padx=1)
    panel.pack(fill=X, padx=(10, 4))
    return panel

# show Output Options dialog ---------------------------------------------------
def out_opt_dlg(root, opt):
    texts = ('Output Paths (File: local_path[::S=tint], TCP: [addr]:port)',
        'PVT Solutions (NMEA 0183)', 'OBS and NAV Data (RTCM3)',
        'Receiver Log (CSV Text)', 'IF Data Log (RAW8, RAW16 or RAW32)',
        'Output Receiver Log Types', 'Keywords Replacement in Path',
        '%Y=Year(yyyy) %y=year(yy) %m=month(mm) %d=day(dd)',
        '%h=hour(00-23) %M=minute(00-59) %S=second(00-59)')
    opt_new = out_opt_new(opt)
    dlg = modal_dlg_new(root, 480, 520, 'Output Options')
    panel1 = Frame(dlg.panel, bg=BG_COLOR, relief=GROOVE, borderwidth=2)
    panel1.pack(fill=X, pady=2)
    ttk.Label(panel1, text=texts[0], justify=LEFT).pack(fill=X, padx=10,
        pady=(4, 8))
    path_panel_new(panel1, texts[1], out=1, var_path=opt_new.path[0],
        var_ena=opt_new.path_ena[0], types=[('NMEA File', '*.nmea'), ('All', '*.*')])
    path_panel_new(panel1, texts[2], out=1, var_path=opt_new.path[1],
       var_ena=opt_new.path_ena[1], types=[('RTCM3 File', '*.rtcm3'), ('All', '*.*')])
    path_panel_new(panel1, texts[3], out=1, var_path=opt_new.path[2],
       var_ena=opt_new.path_ena[2], types=[('Log File', '*.log'), ('All', '*.*')])
    path_panel_new(panel1, texts[4], out=1, var_path=opt_new.path[3],
       var_ena=opt_new.path_ena[3], types=[('Raw IF Log', '*.bin'), ('All', '*.*')])
    panel2 = Frame(dlg.panel, bg=BG_COLOR, relief=GROOVE, borderwidth=2)
    panel2.pack(fill=X, pady=2)
    ttk.Label(panel2, text=texts[5], justify=LEFT).pack(fill=X, padx=10, pady=2)
    panel3 = Frame(panel2, bg=BG_COLOR)
    panel3.pack(pady=4)
    for i in range(len(opt.log_sel)):
        ttk.Checkbutton(panel3, text=opt.log[i], variable=opt_new.log_sel[i]
            ).pack(side=LEFT, padx=2)
    panel4 = Frame(dlg.panel, bg=BG_COLOR, relief=GROOVE, borderwidth=2)
    panel4.pack(fill=X, pady=2)
    ttk.Label(panel4, text=texts[6], justify=LEFT).pack(fill=X, padx=10, pady=2)
    ttk.Label(panel4, text=texts[7], justify=LEFT).pack(fill=X, padx=26, pady=2)
    ttk.Label(panel4, text=texts[8], justify=LEFT).pack(fill=X, padx=26, pady=(2, 8))
    root.wait_window(dlg.win)
    return opt_new if dlg.ok else opt

# show Signal Options dialog ---------------------------------------------------
def sig_opt_dlg(root, opt):
    opt_new = sig_opt_new(opt)
    dlg = modal_dlg_new(root, 480, 560, 'Signal Options')
    panel1 = Frame(dlg.panel, width=450, bg=BG_COLOR, relief=GROOVE, borderwidth=2)
    panel1.pack(pady=2)
    labels_panel(panel1, ('System', 'Satellite No'), (20, 90)).pack(fill=X, pady=(4, 2))
    link_label_new(panel1, text='GNSS Signals', link=SIG_LINK, font=FONT).place(x=265, y=4)
    for i in range(len(opt_new.sys)):
        ttk.Separator(panel1, orient=HORIZONTAL).pack(fill=X, pady=(0, 2))
        sig_opt_panel(panel1, opt_new, i).pack(padx=(10, 4), pady=(4, 2))
    panel2 = Frame(dlg.panel, width=450, bg=BG_COLOR, relief=GROOVE, borderwidth=2)
    panel2.pack(fill=X, pady=2)
    ttk.Label(panel2, text='RF CH Assignments (<sig>:<ch>[-<ch>][,...][ ...])').pack(
        fill=X, padx=10, pady=4)
    inp_panel_new(panel2, '', opt_new.sig_rfch, width=500, justify=LEFT).pack(
        padx=(0, 4), pady=(2, 6))
    root.wait_window(dlg.win)
    return opt_new if dlg.ok else opt

# generate Signal Option panel -------------------------------------------------
def sig_opt_panel(root, opt, i):
    var = opt.sys_sel[i]
    state = NORMAL if var.get() else DISABLED
    ns = len(opt.sig[i])
    h = height=24 + (ns - 1) // 4 * 20
    panel = Frame(root, width=445, height=h, bg=BG_COLOR)
    ttk.Checkbutton(panel, width=8, text=opt.sys[i], variable=var,
        command=lambda: on_sig_opt_change(panel, var)).place(x=0, y=0)
    ttk.Entry(panel, width=9, justify=CENTER, textvariable=opt.satno[i],
        state=state, font=FONT).place(x=80, y=0)
    for j in range(ns):
        x, y = 165 + (j % 4) * 64, (j // 4) * 20
        ttk.Checkbutton(panel, width=8, text=opt.sig[i][j], state=state,
            variable=opt.sig_sel[i][j]).place(x=x, y=y)
    return panel

# signal option change callback -------------------------------------------------
def on_sig_opt_change(p, var):
    for c in p.winfo_children()[1:]:
        c.configure(state=NORMAL if var.get() else DISABLED)

# show System Options dialog ---------------------------------------------------
def sys_opt_dlg(root, opt):
    labels = ('Epoch Interval for PVT (s)', 'Max Epoch Lag for PVT (s)',
        'Elevation Mask for PVT (\xb0)', 'Correlator Spacing (chip)',
        'Integration Time for Acquisition (s)', 'Integration Time for DLL (s)',
        'DLL Loop Filter Bandwidth (Hz)', 'PLL Loop Filter Bandwidth (Hz)',
        'FLL Loop Filter Bandwidth Wide (Hz)',
        'FLL Loop Filter Bandwidth Narrow (Hz)',
        'Max Doppler Frequency to Search Signal (Hz)',
        'C/N0 Threshold for Signal Locked (dB-Hz)',
        'C/N0 Threshold for Signal Lost (dB-Hz)', 'Bump Jump for BOC Modulation')
    opt_new = sys_opt_new(opt)
    dlg = modal_dlg_new(root, 420, 540, 'System Options')
    panel1 = Frame(dlg.panel, bg=BG_COLOR, relief=GROOVE, borderwidth=2)
    panel1.pack(fill=X, pady=2, ipady=1)
    sel_panel_new(panel1, labels[0], sels=('0.1', '0.2', '0.5', '1.0',
        '2.0', '5.0'), var=opt_new.epoch, width=8)
    sel_panel_new(panel1, labels[1], sels=('0.1', '0.2', '0.3', '0.5', '0.75',
        '1.0'), var=opt_new.lag_epoch, width=8)
    sel_panel_new(panel1, labels[2], sels=('5', '10', '15', '20', '25',
        '30'), var=opt_new.el_mask, width=8)
    panel2 = Frame(dlg.panel, bg=BG_COLOR, relief=GROOVE, borderwidth=2)
    panel2.pack(fill=X, pady=2, ipady=1)
    sel_panel_new(panel2, labels[3], sels=('0.05', '0.1', '0.2', '0.25', '0.5',
        '0.75', '1.0'), var=opt_new.sp_corr, width=8)
    sel_panel_new(panel2, labels[4], sels=('0.005', '0.01', '0.02', '0.05',
        '0.1', '0.2'), var=opt_new.t_acq, width=8)
    sel_panel_new(panel2, labels[5], sels=('0.005', '0.01', '0.02', '0.05',
        '0.1', '0.2'), var=opt_new.t_dll, width=8)
    sel_panel_new(panel2, labels[6], sels=('0.1', '0.15', '0.2', '0.25',
        '0.3', '0.4', '0.5', '0.75', '1.0'), var=opt_new.b_dll, width=8)
    sel_panel_new(panel2, labels[7], sels=('1.0', '2.5', '5.0', '7.5', '10.0',
        '15.0', '20.0'), var=opt_new.b_pll, width=8)
    sel_panel_new(panel2, labels[8], sels=('1.0', '2.5', '5.0', '7.5', '10.0',
        '15.0', '20.0'), var=opt_new.b_fll_w, width=8)
    sel_panel_new(panel2, labels[9], sels=('0.5', '1.0', '2.0', '3.0', '4.0',
        '5.0', '6.0'), var=opt_new.b_fll_n, width=8)
    sel_panel_new(panel2, labels[10], sels=('3000', '5000', '7000', '10000',
        '20000'), var=opt_new.max_dop, width=8)
    sel_panel_new(panel2, labels[11], sels=('31.0', '32.0', '33.0', '34.0',
        '35.0', '36.0', '37.0'), var=opt_new.thres_cn0_l, width=8)
    sel_panel_new(panel2, labels[12], sels=('27.0', '28.0', '29.0', '30.0',
        '31.0', '32.0', '33.0'), var=opt_new.thres_cn0_u, width=8)
    sel_panel_new(panel2, labels[13], sels=('OFF', 'ON'), var=opt_new.bump_jump,
        width=8)
    path_panel_new(dlg.panel, 'FFTW Wisdom Path', out=0,
        var_path=opt_new.fftw_wisdom_path)
    root.wait_window(dlg.win)
    return opt_new if dlg.ok else opt

