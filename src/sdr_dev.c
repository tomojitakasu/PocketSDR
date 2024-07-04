// 
//  Pocket SDR C Library - GNSS SDR Device Functions.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2021-10-20  0.1  new
//  2022-01-04  0.2  support CyUSB on Windows
//  2022-01-13  1.0  rise process/thread priority for Windows
//  2022-05-23  1.1  change coding style
//  2022-07-08  1.2  fix a bug
//  2022-08-08  1.3  support Spider SDR
//  2023-12-25  1.3  modify taskname for AvSetMmThreadCharacteristicsA()
//  2024-04-04  1.4  refactored
//  2024-04-13  1.5  suppport Pocket SDR FE 4CH
//  2024-05-28  1.6  delete API sdr_dev_info()
//  2024-06-29  1.7  add API sdr_dev_get_info(), sdr_dev_set_gain(),
//                   sdr_dev_get_gain()
//
#include "pocket_sdr.h"
#ifdef WIN32
#include <avrt.h>
#endif

// constants and macros --------------------------------------------------------
#define BUFF_SIZE       (SDR_SIZE_BUFF * SDR_MAX_BUFF)
#define TO_TRANSFER     3000    // USB transfer timeout (ms)

// read MAX2771 status ---------------------------------------------------------
static int read_MAX2771_stat(sdr_dev_t *dev, int ch, double fx, double *fs,
    double *fo, int *IQ)
{
    static const double ratio[8] = {2.0, 0.25, 0.5, 1.0, 4.0};
    uint8_t data[4];
    uint32_t reg[11], ENIQ, INT_PLL, NDIV, RDIV, FDIV, REFDIV, FCLKIN, ADCCLK;
    uint32_t REFCLK_L, REFCLK_M, ADCCLK_L, ADCCLK_M, PREFRACDIV;
    
    for (int i = 0; i < 11; i++) {
        if (!sdr_usb_req(dev->usb, 0, SDR_VR_REG_READ, (ch << 8) + i, data,
            4)) {
            return 0;
        }
        for (int j = 0; j < 4; j++) { // swap bytes
            *((uint8_t *)(reg + i) + j) = data[3 - j];
        }
    }
    ENIQ     = (reg[ 1] >> 27) & 0x1;
    INT_PLL  = (reg[ 3] >>  3) & 0x1;
    NDIV     = (reg[ 4] >> 13) & 0x7FFF;
    RDIV     = (reg[ 4] >>  3) & 0x3FF;
    FDIV     = (reg[ 5] >>  8) & 0xFFFFF;
    REFDIV   = (reg[ 3] >> 29) & 0x7;
    FCLKIN   = (reg[ 7] >>  3) & 0x1;
    ADCCLK   = (reg[ 7] >>  2) & 0x1;
    REFCLK_L = (reg[ 7] >> 16) & 0xFFF;
    REFCLK_M = (reg[ 7] >>  4) & 0xFFF;
    ADCCLK_L = (reg[10] >> 16) & 0xFFF;
    ADCCLK_M = (reg[10] >>  4) & 0xFFF;
    PREFRACDIV = (reg[0xA] >>  3) & 0x1;
    if (ch == 0) {
        *fs = !PREFRACDIV ? fx : fx * REFCLK_L / (4096.0 - REFCLK_M + REFCLK_L);
        *fs *= ADCCLK  ? 1.0 : ratio[REFDIV];
        *fs *= !FCLKIN ? 1.0 : ADCCLK_L / (4096.0 - ADCCLK_M + ADCCLK_L);
    }
    *fo = fx / RDIV * (INT_PLL ? NDIV : NDIV + FDIV / 1048576.0);
    *IQ = ENIQ ? 2 : 1;
    return 1;
}

// read MAX2769B status --------------------------------------------------------
static int read_MAX2769B_stat(sdr_dev_t *dev, int ch, double fx, double *fs,
    double *fo, int *IQ)
{
    static const double ratio[8] = {2.0, 0.25, 0.5, 1.0};
    uint8_t data[4];
    uint32_t reg[8], ENIQ, INT_PLL, NDIV, RDIV, FDIV, REFDIV, L_CNT, M_CNT;
    uint32_t FCLKIN, ADCCLK;
    
    for (int i = 0; i < 8; i++) {
        if (!sdr_usb_req(dev->usb, 0, SDR_VR_REG_READ, (ch << 8) + i, data,
            4)) {
            return 0;
        }
        for (int j = 0; j < 4; j++) { // swap bytes
            *((uint8_t *)(reg + i) + j) = data[3 - j];
        }
    }
    ENIQ    = (reg[1] >> 27) & 0x1;
    INT_PLL = (reg[3] >>  3) & 0x1;
    NDIV    = (reg[4] >> 13) & 0x7FFF;
    RDIV    = (reg[4] >>  3) & 0x3FF;
    FDIV    = (reg[5] >>  8) & 0xFFFFF;
    REFDIV  = (reg[3] >> 21) & 0x3;
    L_CNT   = (reg[7] >> 16) & 0xFFF;
    M_CNT   = (reg[7] >>  4) & 0xFFF;
    FCLKIN  = (reg[7] >>  3) & 0x1;
    ADCCLK  = (reg[7] >>  2) & 0x1;
    if (ch == 0) {
        *fs = fx * (ADCCLK ? 1.0 : ratio[REFDIV]);
        *fs *= !FCLKIN ? 1.0 : L_CNT / (4096.0 - M_CNT + L_CNT);
    }
    *fo = fx / RDIV * (INT_PLL ? NDIV : NDIV + FDIV / 1048576.0);
    *IQ = ENIQ ? 2 : 1;
    return 1;
}

#ifdef WIN32

// rise process/thread priority ------------------------------------------------
static void rise_pri(void)
{
    if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        fprintf(stderr, "SetPriorityClass error (%d)\n", (int)GetLastError());
    }
    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
        fprintf(stderr, "SetThreadPriority error (%d)\n", (int)GetLastError());
    }
    DWORD task = 0;
    HANDLE h = AvSetMmThreadCharacteristicsA("DisplayPostProcessing", &task);
    
    if (h == 0) {
        fprintf(stderr, "AvSetMmThreadCharacteristicsA error (%d)\n",
            (int)GetLastError());
    }
    else if (!AvSetMmThreadPriority(h, AVRT_PRIORITY_CRITICAL)) {
        fprintf(stderr, "AvSetMmThreadPriority error (%d)\n",
            (int)GetLastError());
    }
}

// get bulk transfer endpoint --------------------------------------------------
static CCyBulkEndPoint *get_bulk_ep(sdr_usb_t *usb, int ep)
{
    for (int i = 0; i < usb->EndPointCount(); i++) {
        if (usb->EndPoints[i]->Attributes == 2 &&
            usb->EndPoints[i]->Address == ep) {
            return (CCyBulkEndPoint *)usb->EndPoints[i];
        }
    }
    return NULL;
}

// USB event handler thread ----------------------------------------------------
static void *event_handler(void *arg)
{
    sdr_dev_t *dev = (sdr_dev_t *)arg;
    CCyBulkEndPoint *ep;
    uint8_t *ctx[SDR_MAX_BUFF] = {0};
    OVERLAPPED ov[SDR_MAX_BUFF] = {{0,0,0,0,0}};
    long len = SDR_SIZE_BUFF;
    
    // rise process/thread priority
    rise_pri();
    
    if (!(ep = get_bulk_ep(dev->usb, SDR_DEV_EP))) {
        fprintf(stderr, "bulk endpoint get error ep=0x%02X\n", SDR_DEV_EP);
        return 0;
    }
    ep->SetXferSize(SDR_SIZE_BUFF);
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        ov[i].hEvent = CreateEvent(NULL, false, false, NULL);
        ctx[i] = ep->BeginDataXfer(dev->buff + len * i, len, &ov[i]); 
    }
    for (int i = 0; dev->state; ) {
        if (!ep->WaitForXfer(&ov[i], TO_TRANSFER)) {
            fprintf(stderr, "bulk transfer timeout\n");
            continue;
        }
        if (!ep->FinishDataXfer(dev->buff + len * i, len, &ov[i], ctx[i])) {
            fprintf(stderr, "bulk transfer error\n");
            break;
        }
        ctx[i] = ep->BeginDataXfer(dev->buff + len * i, len, &ov[i]);
        pthread_mutex_lock(&dev->mtx);
        dev->wp += len;
        pthread_mutex_unlock(&dev->mtx);
        i = (i + 1) % SDR_MAX_BUFF;
    }
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        ep->FinishDataXfer(dev->buff + len * i, len, &ov[i], ctx[i]);
        CloseHandle(ov[i].hEvent);
    }
    return 0;
}

#else // WIN32

// USB bulk transfer callback --------------------------------------------------
static void transfer_cb(struct libusb_transfer *transfer)
{
    sdr_dev_t *dev = (sdr_dev_t *)transfer->user_data;
    
    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "libusb bulk transfer error (%d)\n", transfer->status);
    }
    pthread_mutex_lock(&dev->mtx);
    dev->wp += SDR_SIZE_BUFF;
    pthread_mutex_unlock(&dev->mtx);
    
    libusb_submit_transfer(transfer);
}

// USB event handler thread ----------------------------------------------------
static void *event_handler(void *arg)
{
    sdr_dev_t *dev = (sdr_dev_t *)arg;
    struct sched_param param = {99};
    struct timeval to = {0, 100000};
    
    // set thread scheduling real-time
    if (pthread_setschedparam(dev->thread, SCHED_RR, &param)) {
        fprintf(stderr, "set thread scheduling error\n");
    }
    while (dev->state) {
        if (libusb_handle_events_timeout(NULL, &to)) continue;
    }
    return NULL;
}

#endif // WIN32

//------------------------------------------------------------------------------
//  Open a SDR device.
//
//  args:
//      bus         (I)   USB bus number of SDR device  (-1:any)
//      port        (I)   USB port number of SDR device (-1:any)
//
//  return
//      SDR device pointer (NULL: error)
//
sdr_dev_t *sdr_dev_open(int bus, int port)
{
    sdr_dev_t *dev = (sdr_dev_t *)sdr_malloc(sizeof(sdr_dev_t));
    uint16_t vid[] = {SDR_DEV_VID, SDR_DEV_VID};
    uint16_t pid[] = {SDR_DEV_PID1, SDR_DEV_PID2};
    
    if (!(dev->usb = sdr_usb_open(bus, port, vid, pid, 2))) {
        fprintf(stderr, "No device found. BUS=%d PORT=%d VID=%04X PID=%04X,%04X\n",
            bus, port, SDR_DEV_VID, SDR_DEV_PID1, SDR_DEV_PID2);
        sdr_free(dev);
        return NULL;
    }
    dev->buff = (uint8_t *)sdr_malloc(BUFF_SIZE);
    
#ifndef WIN32
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        if (!(dev->transfer[i] = libusb_alloc_transfer(0))) {
            fprintf(stderr, "libusb_alloc_transfer(%d) error\n", i);
            sdr_usb_close(dev->usb);
            sdr_free(dev);
            return NULL;
        }
    }
#endif
    pthread_mutex_init(&dev->mtx, NULL);
    return dev;
}

//------------------------------------------------------------------------------
//  Close SDR device.
//
//  args:
//      dev         (I)   SDR device
//
//  return
//      none
//
void sdr_dev_close(sdr_dev_t *dev)
{
    sdr_usb_close(dev->usb);
#ifndef WIN32
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        libusb_free_transfer(dev->transfer[i]);
    }
#endif
    sdr_free(dev->buff);
    sdr_free(dev);
}

//------------------------------------------------------------------------------
//  Start the SDR device.
//
//  args:
//      dev         (I)   USB device pointer
//
//  return
//      status (1: OK, 0: error)
//
int sdr_dev_start(sdr_dev_t *dev)
{
    if (dev->state) return 0;
    
#ifndef WIN32
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        int ret;
        libusb_fill_bulk_transfer(dev->transfer[i], dev->usb->h, SDR_DEV_EP,
            dev->buff + SDR_SIZE_BUFF * i, SDR_SIZE_BUFF, transfer_cb, dev,
            TO_TRANSFER);
        if ((ret = libusb_submit_transfer(dev->transfer[i]))) {
            fprintf(stderr, "libusb_submit_transfer(%d) error (%d)\n", i, ret);
            return 0;
        }
    }
#endif
    sdr_usb_req(dev->usb, 0, SDR_VR_START, 0, NULL, 0);
    
    dev->state = 1;
    dev->rp = dev->wp = 0;
    pthread_create(&dev->thread, NULL, event_handler, dev);
    return 1;
}

//------------------------------------------------------------------------------
//  Stop the SDR device.
//
//  args:
//      dev         (I)   USB device pointer
//
//  return
//      status (1: OK, 0: error)
//
int sdr_dev_stop(sdr_dev_t *dev)
{
    if (!dev->state) return 0;
    
    dev->state = 0;
    pthread_join(dev->thread, NULL);
    sdr_usb_req(dev->usb, 0, SDR_VR_STOP, 0, NULL, 0);
#ifndef WIN32
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        libusb_cancel_transfer(dev->transfer[i]);
    }
#endif
    return 1;
}

//------------------------------------------------------------------------------
//  Read of IF data (non-block). Immediately returned with return value 0 if
//  insufficient data received.
//
//  args:
//      dev         (I)   USB device pointer
//      buff        (O)   IF data buffer
//      size        (I)   IF data size to be read (bytes)
//
//  return
//      data size read (0: insufficent data) (bytes)
//
int sdr_dev_read(sdr_dev_t *dev, uint8_t *buff, int size)
{
    pthread_mutex_lock(&dev->mtx);
    int64_t wp = dev->wp;
    pthread_mutex_unlock(&dev->mtx);
    
    if (wp < dev->rp + size) {
        return 0;
    }
    int rp = (int)(dev->rp % BUFF_SIZE);
    
    if (rp + size <= BUFF_SIZE) {
        memcpy(buff, dev->buff + rp, size);
    }
    else {
        memcpy(buff, dev->buff + rp, BUFF_SIZE - rp);
        memcpy(buff + BUFF_SIZE - rp, dev->buff, size - BUFF_SIZE + rp);
    }
    dev->rp += size;
    return size;
}

//------------------------------------------------------------------------------
//  Get device info of SDR device.
//
//  args:
//      dev         (I)   SDR device
//      fmt         (O)   IF data format (SDR_FMT_???)
//      fs          (O)   sampling frequency (Hz)
//      fo          (O)   LO frequency of each RF channel (Hz)
//      IO          (O)   sampling type of each RF channel (1:I, 2:IQ)
//
//  return
//      number of RF channels (0: error)
//
int sdr_dev_get_info(sdr_dev_t *dev, int *fmt, double *fs, double *fo, int *IQ)
{
    double fss;
    uint8_t data[6];
    int nch = 0;
    
    // read device info and status
    if (!sdr_usb_req(dev->usb, 0, SDR_VR_STAT, 0, data, 6)) {
        return 0;
    }
    int type = (data[3] >> 4) & 1; // 0: Pocket SDR, 1: Spider SDR
    double fx = (((uint16_t)data[1] << 8) + data[2]) * 1e3; // TCXO freq (Hz)
    
    if (type == 1) { // Spider SDR
        *fmt = SDR_FMT_RAW16I;
        nch = data[3] & 0xF;
        for (int i = 0; i < nch; i++) {
            if (!read_MAX2769B_stat(dev, i, fx, &fss, fo + i, IQ + i)) return 0;
            if (i == 0) *fs = fss;
        }
    }
    else { // Pocket SDR FE
        int ver = data[0] >> 4;
        *fmt = (ver <= 2) ? SDR_FMT_RAW8 : SDR_FMT_RAW16; // 2CH : 4CH
        nch = (ver <= 2) ? 2 : 4;
        for (int i = 0; i < nch; i++) {
            if (!read_MAX2771_stat(dev, i, fx, &fss, fo + i, IQ + i)) return 0;
            if (i == 0) *fs = fss;
        }
    }
    return nch;
}

//------------------------------------------------------------------------------
//  Set LNA gain of SDR device.
//
//  args:
//      dev         (I)   SDR device
//      ch          (I)   RF channel (0:CH1, 1:CH2, ...)
//      gain        (I)   LNA gain (0: AGC, 1-64: gain dB)
//
//  return
//      status (1: OK, 0: error)
//
int sdr_dev_set_gain(sdr_dev_t *dev, int ch, int gain)
{
    uint8_t data[6], reg1[4], reg2[4];
    
    if (!dev->state) return 0;
    
    // read device info
    if (!sdr_usb_req(dev->usb, 0, SDR_VR_STAT, 0, data, 6)) {
        return 0;
    }
    int ver = data[0] >> 4, nch = (ver <= 2) ? 2 : 4;
    
    if (ch < 0 || ch >= nch) {
        return 0;
    }
    // read MAX2771 registers
    if (!sdr_usb_req(dev->usb, 0, SDR_VR_REG_READ, (ch << 8) + 1, reg1, 4) ||
        !sdr_usb_req(dev->usb, 0, SDR_VR_REG_READ, (ch << 8) + 2, reg2, 4)) {
        return 0;
    }
    if (gain > 0) { // manual gain
        reg1[2] = (reg1[2] & ~0x18) + (2 << 3); // AGCMODE = 2
        reg2[0] = (reg2[0] & ~0x0F) + (((gain - 1) >> 2) & 0x0F); // GAININ[5:2]
        reg2[1] = (reg2[1] & ~0xC0) + (((gain - 1) << 6) & 0xC0); // GAININ[1:0]
    }
    else { // AGC
        reg1[2] = (reg1[2] & ~0x18); // AGCMODE = 0
    }
    // write MAX2771 registers
    return sdr_usb_req(dev->usb, 1, SDR_VR_REG_WRITE, (ch << 8) + 1, reg1, 4) &&
           sdr_usb_req(dev->usb, 1, SDR_VR_REG_WRITE, (ch << 8) + 2, reg2, 4);
}

//------------------------------------------------------------------------------
//  Get LNA gain of SDR device.
//
//  args:
//      dev         (I)   SDR device
//      ch          (I)   RF channel (0:CH1, 1:CH2, ...)
//
//  return
//      LNA gain (0: AGC, 1-64: LNA gain dB, -1: error)
//
int sdr_dev_get_gain(sdr_dev_t *dev, int ch)
{
    uint8_t data[6], reg1[4], reg2[4];
    
    if (!dev->state) return 0;
    
    // read device info
    if (!sdr_usb_req(dev->usb, 0, SDR_VR_STAT, 0, data, 6)) {
        return -1;
    }
    int ver = data[0] >> 4, nch = (ver <= 2) ? 2 : 4;
    
    if (ch < 0 || ch >= nch) {
        return -1;
    }
    // read MAX2771 registers
    if (!sdr_usb_req(dev->usb, 0, SDR_VR_REG_READ, (ch << 8) + 1, reg1, 4) ||
        !sdr_usb_req(dev->usb, 0, SDR_VR_REG_READ, (ch << 8) + 2, reg2, 4)) {
        return -1;
    }
    if (((reg1[2] >> 3) & 0x03) == 2) { // manual gain
        return ((reg2[0] & 0x0F) << 2) + (reg2[1] >> 6) + 1;
    }
    else { // AGC
        return 0;
    }
}
