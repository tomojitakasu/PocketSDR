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
//
#include "pocket_sdr.h"
#include "pocket_dev.h"
#ifdef WIN32
#include <avrt.h>
#endif

// constants and macros --------------------------------------------------------
#define BUFF_SIZE       (SDR_SIZE_BUFF * SDR_MAX_BUFF)
#define TO_TRANSFER     3000    // USB transfer timeout (ms)

// read MAX2771 status ---------------------------------------------------------
static int read_MAX2771_stat(sdr_dev_t *dev, int ch, double fx)
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
        dev->fs = !PREFRACDIV ?
            fx : fx * REFCLK_L / (4096.0 - REFCLK_M + REFCLK_L);
        dev->fs *= ADCCLK  ? 1.0 : ratio[REFDIV];
        dev->fs *= !FCLKIN ? 1.0 : ADCCLK_L / (4096.0 - ADCCLK_M + ADCCLK_L);
    }
    dev->fo[ch] = fx / RDIV * (INT_PLL ? NDIV : NDIV + FDIV / 1048576.0);
    dev->IQ[ch] = ENIQ ? 2 : 1;
    return 1;
}

// read MAX2769B status --------------------------------------------------------
static int read_MAX2769B_stat(sdr_dev_t *dev, int ch, double fx)
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
        dev->fs = fx * (ADCCLK ? 1.0 : ratio[REFDIV]);
        dev->fs *= !FCLKIN ? 1.0 : L_CNT / (4096.0 - M_CNT + L_CNT);
    }
    dev->fo[ch] = fx / RDIV * (INT_PLL ? NDIV : NDIV + FDIV / 1048576.0);
    dev->IQ[ch] = ENIQ ? 2 : 1;
    return 1;
}

// read device info ------------------------------------------------------------
static int read_dev_info(sdr_dev_t *dev)
{
    uint8_t data[6];
    
    // read device info and status
    if (!sdr_usb_req(dev->usb, 0, SDR_VR_STAT, 0, data, 6)) {
        return 0;
    }
    int type = (data[3] >> 4) & 1; // 0: Pocket SDR, 1: Spider SDR
    double fx = (((uint16_t)data[1] << 8) + data[2]) * 1e3;
    
    if (type == 1) { // Spider SDR
        int nch = data[3] & 0xF;
        for (int i = 0; i < nch; i++) {
            if (!read_MAX2769B_stat(dev, i, fx)) return 0;
        }
        dev->fmt = SDR_FMT_RAW16I;
    }
    else { // Pocket SDR
        int ver = data[0] >> 4, nch = (ver <= 2) ? 2 : 4;
        for (int i = 0; i < nch; i++) {
            if (!read_MAX2771_stat(dev, i, fx)) return 0;
        }
        dev->fmt = (ver <= 2) ? SDR_FMT_RAW8 : SDR_FMT_RAW16; // 2CH : 4CH
    }
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
static DWORD WINAPI event_handler(void *arg)
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
    if (dev->fmt == SDR_FMT_RAW16) {
        (void)sdr_usb_req(dev->usb, 0, SDR_VR_START, 0, NULL, 0);
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
        dev->wp += len;
        i = (i + 1) % SDR_MAX_BUFF;
    }
    if (dev->fmt == SDR_FMT_RAW16) {
        (void)sdr_usb_req(dev->usb, 0, SDR_VR_STOP, 0, NULL, 0);
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
    dev->wp += SDR_SIZE_BUFF;
    libusb_submit_transfer(transfer);
}

// USB event handler thread ----------------------------------------------------
static void *event_handler(void *arg)
{
    sdr_dev_t *dev = (sdr_dev_t *)arg;
    struct sched_param param = {99};
    struct timeval to = {0, 100000};
    int ret;
    
    // set thread scheduling real-time
    if (pthread_setschedparam(dev->thread, SCHED_RR, &param)) {
        fprintf(stderr, "set thread scheduling error\n");
    }
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        if ((ret = libusb_submit_transfer(dev->transfer[i]))) {
            fprintf(stderr, "libusb_submit_transfer(%d) error (%d)\n", i, ret);
            return NULL;
        }
    }
    if (dev->fmt == SDR_FMT_RAW16) {
        (void)sdr_usb_req(dev->usb, 0, SDR_VR_START, 0, NULL, 0);
    }
    while (dev->state) {
        if (libusb_handle_events_timeout(NULL, &to)) continue;
    }
    if (dev->fmt == SDR_FMT_RAW16) {
        (void)sdr_usb_req(dev->usb, 0, SDR_VR_STOP, 0, NULL, 0);
    }
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        libusb_cancel_transfer(dev->transfer[i]);
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
    
    if (!(dev->usb = sdr_usb_open(bus, port, SDR_DEV_VID, SDR_DEV_PID1)) &&
        !(dev->usb = sdr_usb_open(bus, port, SDR_DEV_VID, SDR_DEV_PID2))) {
        fprintf(stderr, "No device found. BUS=%d PORT=%d ID=%04X:%04X/%04X\n",
            bus, port, SDR_DEV_VID, SDR_DEV_PID1, SDR_DEV_PID2);
        sdr_free(dev);
        return NULL;
    }
    // read device info
    if (!read_dev_info(dev)) {
        fprintf(stderr, "Device info read error.\n");
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
        libusb_fill_bulk_transfer(dev->transfer[i], dev->usb->h, SDR_DEV_EP,
            dev->buff + SDR_SIZE_BUFF * i, SDR_SIZE_BUFF, transfer_cb, dev,
            TO_TRANSFER);
    }
#endif
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
    dev->state = 1;
    dev->rp = dev->wp = 0;
#ifdef WIN32
    dev->thread = CreateThread(NULL, 0, event_handler, dev, 0, NULL);
#else
    pthread_create(&dev->thread, NULL, event_handler, dev);
#endif
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
#ifdef WIN32
    dev->state = 0;
    WaitForSingleObject(dev->thread, 10000);
    CloseHandle(dev->thread);
#else
    dev->state = 0;
    pthread_join(dev->thread, NULL);
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
    int rp = (int)(dev->rp % BUFF_SIZE);
    
    if (dev->wp < dev->rp + size) {
        return 0;
    }
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

