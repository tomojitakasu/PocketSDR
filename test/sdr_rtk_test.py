#!/usr/bin/env python3
#
#  unit test driver for sdr_rtk.py
#
import sys, time, math
sys.path.append('../python')
import numpy as np
import sdr_rtk

# test sdr_rtk.satno(), satsys() -----------------------------------------------
def test_01():
    for sat in range(0, 256):
        sys, prn = sdr_rtk.satsys(sat)
        satno = sdr_rtk.satno(sys, prn)
        #print('sat=%3d sys=%02X prn=%3d' % (sat, sys, prn))
        if sys != sdr_rtk.SYS_NONE and sat != satno:
            print('satsys: sat=%3d NG' % (sat))
            exit()
    print('test_01: OK')

# test sdr_rtk.satid2no(), sano2id() -------------------------------------------
def test_02():
    for sat in range(0, 256):
        id = sdr_rtk.satno2id(sat)
        satno = sdr_rtk.satid2no(id)
        #print('sat=%03d id=[%s]' % (sat, id))
        if id and sat != satno:
            print('satno2id: sat=%3d NG' % (sat))
            exit()
    print('test_02: OK')

# test sdr_rtk.obs2code(), code2obs() ------------------------------------------
def test_03():
    for code in range(0, 256):
        obs = sdr_rtk.code2obs(code)
        obscode = sdr_rtk.obs2code(obs)
        #print('code=%03d obs=[%s]' % (code, obs))
        if obs and code != obscode:
            print('obs2code: code=%3d NG' % (code))
            exit()
    print('test_03: OK')

# test sdr_rtk.epoch2time(), time2epoch() --------------------------------------
def test_04():
    ep = (np.array([1980, 1, 8, 12, 0, 0.000], dtype='double'),
          np.array([2000, 2,28, 23,59,59.999], dtype='double'),
          np.array([2000, 2,29,  0, 0, 0.000], dtype='double'),
          np.array([2032,12,31, 23,59,59.999], dtype='double'))
    
    for e in ep:
        time = sdr_rtk.epoch2time(e)
        epoch = sdr_rtk.time2epoch(time)
        #print('time=%s epoch=%04.0f/%02.0f/%02.0f %02.0f:%02.0f:%06.3f' % (
        #    sdr_rtk.time2str(time, 6),
        #    epoch[0], epoch[1], epoch[2], epoch[3], epoch[4], epoch[5]))
        if not np.all(e == epoch):
            print('epcoh2time: time=%s NG' % (sdr_rtk.time2str(time, 6)))
            exit()
    print('test_04: OK')

# test sdr_rtk.gpst2time(), time2gpst() ----------------------------------------
def test_05():
    ep = (np.array([1980, 1, 8, 12, 0, 0.000], dtype='double'),
          np.array([2000, 2,28, 23,59,59.999], dtype='double'),
          np.array([2000, 2,29,  0, 0, 0.000], dtype='double'),
          np.array([2032,12,31, 23,59,59.999], dtype='double'))
    
    for e in ep:
        time = sdr_rtk.epoch2time(e)
        week, tow = sdr_rtk.time2gpst(time)
        timeg = sdr_rtk.gpst2time(week, tow)
        dt = sdr_rtk.timediff(time, timeg)
        #print('time=%s week=%04d tow=%12.6f' % (
        #    sdr_rtk.time2str(time, 6), week, tow))
        if np.abs(dt) > 1e-9:
            print('gpst2time: time=%s week=%4d tow=%12.6f dt=%.4e NG' % (
                sdr_rtk.time2str(time, 6), week, tow, dt))
            exit()
    print('test_05: OK')

# test sdr_rtk.gpst2utc(), utc2gpst() ------------------------------------------
def test_06():
    ep = (np.array([1980, 1, 8, 12, 0, 0.000], dtype='double'),
          np.array([2000, 2,28, 23,59,59.999], dtype='double'),
          np.array([2000, 2,29,  0, 0, 0.000], dtype='double'),
          np.array([2032,12,31, 23,59,59.999], dtype='double'))
    
    for e in ep:
        gpst = sdr_rtk.epoch2time(e)
        utc = sdr_rtk.gpst2utc(gpst)
        gpstg = sdr_rtk.utc2gpst(utc)
        dt = sdr_rtk.timediff(gpst, gpstg)
        #print('gpst=%s utc=%s' % (sdr_rtk.time2str(gpst, 3), sdr_rtk.time2str(utc, 3)))
        if np.abs(dt) > 1e-9:
            print('gpst2utc: time=%s dt=%.4e NG' % (sdr_rtk.time2str(gpst, 3), dt))
            exit()
    print('test_06: OK')

# test sdr_rtk.timeadd(), timediff() -------------------------------------------
def test_07():
    ep = (np.array([1980, 1, 8, 12, 0, 0.000], dtype='double'),
          np.array([2000, 2,28, 23,59,59.999], dtype='double'),
          np.array([2000, 2,29,  0, 0, 0.000], dtype='double'),
          np.array([2032,12,31, 23,59,59.999], dtype='double'))
    
    for e in ep:
        dt = 12345678.90123
        time1 = sdr_rtk.epoch2time(e)
        time2 = sdr_rtk.timeadd(time1, dt)
        dt1 = sdr_rtk.timediff(time2, time1)
        time3 = sdr_rtk.timeadd(time1, -dt)
        dt2 = sdr_rtk.timediff(time3, time1)
        #print('dt=%.6f %.6f %.6f' % (dt, dt1, dt2))
        if np.abs(dt - dt1) > 1e-9 or np.abs(dt + dt2) > 1e-9:
            print('timeadd: time=%s dt=%.4e NG' % (sdr_rtk.time2str(time1, 3), dt))
            exit()
    print('test_07: OK')

# test sdr_rtk.time2str() ------------------------------------------------------
def test_08():
    ep = (np.array([1980, 1, 8, 12, 0, 0.000], dtype='double'),
          np.array([2000, 2,28, 23,59,59.999], dtype='double'),
          np.array([2000, 2,29,  0, 0, 0.000], dtype='double'),
          np.array([2032,12,31, 23,59,59.999], dtype='double'))
    
    for e in ep:
        time = sdr_rtk.epoch2time(e)
        str1 = sdr_rtk.time2str(time, 3)
        epoch = sdr_rtk.time2epoch(time)
        str2 = '%04.0f/%02.0f/%02.0f %02.0f:%02.0f:%06.3f' % (
            epoch[0], epoch[1], epoch[2], epoch[3], epoch[4], epoch[5])
        #print('time2str: time=%s %s' % (str1, str2))
        if str1 != str2:
            print('time2str: time=%s %s NG' % (str1, str2))
            exit()
    print('test_08: OK')

# test sdr_rtk.readrnx() -------------------------------------------------------
def test_09():
    file1 = 'data/mosaic_20220221_0353.ob_'
    file2 = 'data/mosaic_20220221_0353.obs'
    file3 = 'data/mosaic_20220221_0353.nav'
    
    # no RINEX
    obs, nav = sdr_rtk.readrnx(file1)
    
    if obs or nav:
        print('readrnx: file=%s NG' % (file1))
        
    # RINEX OBS ALL
    obs, nav = sdr_rtk.readrnx(file2)
    
    if not obs or not nav:
        print('readrnx: file=%s NG' % (file2))
        exit()
    
    n = 0
    for data in sdr_rtk.obsget(obs):
        #print('%s sat=%s rcv=%d' % (sdr_rtk.time2str(data.time, 2),
        #    sdr_rtk.satno2id(data.sat), data.rcv))
        n += 1
    if n < 4000:
        print('obsget: file=%s n=%d NG' % (file2, n))
        exit()
    sdr_rtk.obsfree(obs)
    sdr_rtk.navfree(nav)
    
    # RINEX OBS TIME RANGE and INTERVAL
    ts = sdr_rtk.epoch2time([2022,2,1,5,10, 0])
    te = sdr_rtk.epoch2time([2022,2,1,5,20,56])
    obs, nav = sdr_rtk.readrnx(file2, ts=ts, te=te, tint=30.0)
    
    if not obs or not nav:
        print('readrnx: file=%s NG' % (file2))
        exit()
    
    n = 0
    for data in sdr_rtk.obsget(obs):
        #print('%s sat=%s rcv=%d' % (sdr_rtk.time2str(data.time, 2),
        #    sdr_rtk.satno2id(data.sat), data.rcv))
        n += 1
    if n < 1000:
        print('obsget: file=%s n=%d NG' % (file2, n))
        exit()
    sdr_rtk.obsfree(obs)
    sdr_rtk.navfree(nav)
    
    # RINEX NAV
    obs, nav = sdr_rtk.readrnx(file3)
    
    if not obs or not nav:
        print('readrnx: file=%s NG' % (file3))
        exit()
    
    n = 0
    for eph in sdr_rtk.ephget(nav):
        #print('sat=%s toe=%s iode=%3d' % (sdr_rtk.satno2id(eph.sat),
        #    sdr_rtk.time2str(eph.toe, 2), eph.iode))
        n += 1
    if n < 100:
        print('ephget: file=%s n=%d NG' % (file2, n))
        exit()
    
    n = 0
    for geph in sdr_rtk.gephget(nav):
        #print('sat=%s tof=%s frq=%2d' % (sdr_rtk.satno2id(geph.sat),
        #    sdr_rtk.time2str(geph.tof, 2), geph.frq))
        n += 1
    if n < 10:
        print('gephget: file=%s n=%d NG' % (file2, n))
        exit()
    
    sdr_rtk.obsfree(obs)
    sdr_rtk.navfree(nav)
    print('test_09: OK')

# test sdr_rtk.satpos() -------------------------------------------------------
def test_10():
    file = 'data/mosaic_20220221_0353.nav'
    
    # RINEX NAV
    obs, nav = sdr_rtk.readrnx(file)
    
    time = sdr_rtk.epoch2time([2022, 2, 1, 5, 35, 20.123])
    
    n = 0
    for sat in range(1, sdr_rtk.MAXSAT + 1):
        rs, dts, var, svh = sdr_rtk.satpos(time, time, sat, nav)
        if len(rs) >= 6:
            #print('%s %s pos=%13.3f %13.3f %13.3f %13.6f %13.6f %13.6f dtr=%13.3f %9.6f sig=%6.1f svh=%3d' % (
            #    sdr_rtk.time2str(time, 2), sdr_rtk.satno2id(sat), rs[0], rs[1],
            #    rs[2], rs[3], rs[4], rs[5], dts[0] * 1e9, dts[1] * 1e9,
            #    math.sqrt(var), svh))
            n += 1
    
    if n < 60:
        print('satpos: file=%s n=%d NG' % (file, n))
        exit()
    
    sdr_rtk.obsfree(obs)
    sdr_rtk.navfree(nav)
    print('test_10: OK')

# test sdr_rtk.ecef2pos(), pos2ecef() -------------------------------------------
def test_11():
    pos = (np.array([ 40.123 * sdr_rtk.D2R, 135.186 * sdr_rtk.D2R, 24.567], dtype='double'),
           np.array([-40.567 * sdr_rtk.D2R, -55.012 * sdr_rtk.D2R, -3.002], dtype='double'),
           np.array([-89.999 * sdr_rtk.D2R, 179.999 * sdr_rtk.D2R, -0.001], dtype='double'))
    
    for p in pos:
        e = sdr_rtk.pos2ecef(p)
        p2 = sdr_rtk.ecef2pos(e)
        #print('pos=%14.9f %14.9f %10.3f xyz=%14.3f %14.3f %14.3f' % (
        #    p[0] * sdr_rtk.R2D, p[1] * sdr_rtk.R2D, p[2], e[0], e[1], e[2]))
        if np.any(np.abs(p - p2) > 1e-4):
            print('ecef2pos: pos=%.9f %.9f %.9f NG' % (p[0], p[1], p[2]))
            exit()
    print('test_11: OK')

# test sdr_rtk.ecef2enu(), enu2ecef() -------------------------------------------
def test_12():
    pos = (np.array([ 40.123 * sdr_rtk.D2R, 135.186 * sdr_rtk.D2R, 24.567], dtype='double'),
           np.array([-40.567 * sdr_rtk.D2R, -55.012 * sdr_rtk.D2R, -3.002], dtype='double'),
           np.array([-89.999 * sdr_rtk.D2R, 179.999 * sdr_rtk.D2R, -0.001], dtype='double'))
    enu = np.array([-12345.678, 9876543.21, 44455.67], dtype='double')
    
    for p in pos:
        r = sdr_rtk.enu2ecef(p, enu)
        e = sdr_rtk.ecef2enu(p, r)
        #print('pos=%14.9f %14.9f %10.3f enu=%14.6f %14.6f %14.6f' % (
        #    p[0] * sdr_rtk.R2D, p[1] * sdr_rtk.R2D, p[2], e[0], e[1], e[2]))
        if np.any(np.abs(e - enu) > 1e-4):
            print('ecef2enu: pos=%.9f %.9f %.9f NG' % (p[0], p[1], p[2]))
            exit()
    print('test_12: OK')

# test sdr_rtk.satazel(), geodist() ---------------------------------------------
def test_13():
    file = 'data/mosaic_20220221_0353.nav'
    pos = np.array([40.123 * sdr_rtk.D2R, 135.186 * sdr_rtk.D2R, 24.567])
    rr = sdr_rtk.pos2ecef(pos)
    obs, nav = sdr_rtk.readrnx(file)
    time = sdr_rtk.epoch2time([2022, 2, 1, 5, 35, 20.123])
    
    n = 0
    for sat in range(1, sdr_rtk.MAXSAT + 1):
        rs, dts, var, svh = sdr_rtk.satpos(time, time, sat, nav)
        if np.dot(rs, rs) < 1e-3:
            continue
        r, e = sdr_rtk.geodist(rs, rr)
        az, el = sdr_rtk.satazel(pos, e)
        #print('(%2d) %s  r=%13.3f  e=%9.6f %9.6f %9.6f  az=%6.1f el=%5.1f' % (
        #    n + 1, sdr_rtk.satno2id(sat), r, e[0], e[1], e[2],
        #    az * sdr_rtk.R2D, el * sdr_rtk.R2D))
        if r > 2e7:
            n += 1
    if n < 30:
        print('satazel: n=%d NG' % (n))
        exit()
    print('test_13: OK')

# main --------------------------------------------------------------------------
if __name__ == '__main__':
    #sdr_rtk.traceopen('test.trace')
    #sdr_rtk.tracelevel(4)
    
    test_01()
    test_02()
    test_03()
    test_04()
    test_05()
    test_06()
    test_07()
    test_08()
    test_09()
    test_10()
    test_11()
    test_12()
    test_13()
    
    #sdr_rtk.traceclose()
