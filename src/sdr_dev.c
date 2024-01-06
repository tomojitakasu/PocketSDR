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
//
#include "pocket_dev.h"
#ifdef WIN32
#include <avrt.h>
#endif

// constants and macros --------------------------------------------------------
#define TO_TRANSFER     3000    // USB transfer timeout (ms)
#define TO_TR_LIBUSB    10000   // USB transfer timeout for LIBUSB (ms)

// quantization lookup table ---------------------------------------------------
static int8_t LUT[2][2][256];

// generate quantization lookup table ------------------------------------------
static void gen_LUT(void)
{
    static const int8_t val[] = {+1, +3, -1, -3}; // 2bit, sign + magnitude
    
    if (LUT[0][0][0]) return;
    
    for (int i = 0; i < 256; i++) {
        LUT[0][0][i] = val[(i>>0) & 0x3]; // CH1 I
        LUT[0][1][i] = val[(i>>2) & 0x3]; // CH1 Q
        LUT[1][0][i] = val[(i>>4) & 0x3]; // CH2 I
        LUT[1][1][i] = val[(i>>6) & 0x3]; // CH2 Q
    }
}

// read sampling type ----------------------------------------------------------
static int read_sample_type(sdr_dev_t *dev)
{
    uint8_t data[6];
    
    // read device info and status
    if (!sdr_usb_req(dev->usb, 0, SDR_VR_STAT, 0, data, 6)) {
       return 0;
    }
    if ((data[3] >> 4) & 1) { // Spider SDR
        dev->max_ch = data[3] & 0xF;
        for (int i = 0; i < dev->max_ch; i++) {
            dev->IQ[i] = 3; // 16 bit-I
        }
    }
    else { // Pocket SDR
        dev->max_ch = 2;
        for (int i = 0; i < dev->max_ch; i++) {
            // read MAX2771 ENIQ field
            if (!sdr_usb_req(dev->usb, 0, SDR_VR_REG_READ,
                    (uint16_t)((i << 8) + 1), data, 4)) {
               return 0;
            }
            dev->IQ[i] = ((data[0] >> 3) & 1) ? 2 : 1; // 1:8bit-I,2:8bit-IQ
        }
    }
    return 1;
}

#ifdef WIN32

// get bulk transfer endpoint --------------------------------------------------
static sdr_ep_t *get_bulk_ep(sdr_usb_t *usb, int ep)
{
    for (int i = 0; i < usb->EndPointCount(); i++) {
        if (usb->EndPoints[i]->Attributes == 2 &&
            usb->EndPoints[i]->Address == ep) {
            return (sdr_ep_t *)usb->EndPoints[i];
        }
    }
    fprintf(stderr, "No bulk end point ep=%02X\n", ep);
    return NULL;
}

// read buffer -----------------------------------------------------------------
static uint8_t *read_buff(sdr_dev_t *dev)
{
    int rp = dev->rp;
    
    if (rp == dev->wp) {
        return NULL;
    }
    if (rp == (dev->wp + 1) % SDR_MAX_BUFF) {
        fprintf(stderr, "bulk transfer buffer overflow\n");
    }
    dev->rp = (rp + 1) % SDR_MAX_BUFF;
    return dev->buff[rp];
}

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

// event handler thread --------------------------------------------------------
static DWORD WINAPI event_handler(void *arg)
{
    sdr_dev_t *dev = (sdr_dev_t *)arg;
    uint8_t *ctx[SDR_MAX_BUFF] = {0};
    OVERLAPPED ov[SDR_MAX_BUFF] = {0};
    long len = SDR_SIZE_BUFF;
    
    // rise process/thread priority
    rise_pri();
    
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        ov[i].hEvent = CreateEvent(NULL, false, false, NULL);
        ctx[i] = dev->ep->BeginDataXfer(dev->buff[i], len, &ov[i]); 
    }
    for (int i = 0; dev->state; ) {
        if (!dev->ep->WaitForXfer(&ov[i], TO_TRANSFER)) {
            fprintf(stderr, "bulk transfer timeout\n");
            continue;
        }
        if (!dev->ep->FinishDataXfer(dev->buff[i], len, &ov[i], ctx[i])) {
            fprintf(stderr, "bulk transfer error\n");
            break;
        }
        ctx[i] = dev->ep->BeginDataXfer(dev->buff[i], len, &ov[i]);
        dev->wp = i;
        i = (i + 1) % SDR_MAX_BUFF;
    }
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        dev->ep->FinishDataXfer(dev->buff[i], len, &ov[i], ctx[i]);
        CloseHandle(ov[i].hEvent);
    }
    return 0;
}

#else // WIN32

// write ring-buffer -----------------------------------------------------------
static int write_buff(sdr_dev_t *dev, uint8_t *data)
{
    int wp = (dev->wp + 1) % SDR_MAX_BUFF;
    if (wp == dev->rp) {
        return 0;
    }
    dev->buff[wp] = data;
    dev->wp = wp;
    return 1;
}

// read ring-buffer ------------------------------------------------------------
static uint8_t *read_buff(sdr_dev_t *dev)
{
    uint8_t *data;
    
    if (dev->rp == dev->wp) {
        return NULL;
    }
    data = dev->buff[dev->rp];
    dev->rp = (dev->rp + 1) % SDR_MAX_BUFF;
    return data;
}

// USB bulk transfer callback --------------------------------------------------
static void transfer_cb(struct libusb_transfer *transfer)
{
    sdr_dev_t *dev = (sdr_dev_t *)transfer->user_data;
    
    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "USB bulk transfer error (%d)\n", transfer->status);
    }
    else if (!write_buff(dev, transfer->buffer)) {
        fprintf(stderr, "USB bulk transfer buffer overflow\n");
    }
    libusb_submit_transfer(transfer);
}

// USB event handler thread ----------------------------------------------------
static void *event_handler_thread(void *arg)
{
    sdr_dev_t *dev = (sdr_dev_t *)arg;
    struct timeval to = {0, 1000000};
    
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
#ifdef WIN32
    sdr_dev_t *dev = new sdr_dev_t;
    
    if (!(dev->usb = sdr_usb_open(bus, port, SDR_DEV_VID, SDR_DEV_PID))) {
        delete dev;
        return NULL;
    }
    if (!(dev->ep = get_bulk_ep(dev->usb, SDR_DEV_EP))) {
        sdr_usb_close(dev->usb);
        delete dev;
        return NULL;
    }
    if (!read_sample_type(dev)) {
        sdr_usb_close(dev->usb);
        delete dev;
        fprintf(stderr, "Read sampling type error\n");
        return NULL;
    }
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        dev->buff[i] = new uint8_t[SDR_SIZE_BUFF];
    }
    dev->ep->SetXferSize(SDR_SIZE_BUFF);
    gen_LUT();
    
    dev->state = 1;
    dev->rp = dev->wp = 0;
    dev->thread = CreateThread(NULL, 0, event_handler, dev, 0, NULL);
    
    return dev;
    
#else // WIN32
    sdr_dev_t *dev;
    struct sched_param param = {99};
    int ret;
#if 1
    // increase kernel memory size of USB stacks (16 MB-> 256 MB)
    if (system("echo 256 > /sys/module/usbcore/parameters/usbfs_memory_mb\n")) {
        fprintf(stderr, "Kernel memory size setting error\n");
    }
#endif
    if (!(dev = (sdr_dev_t *)malloc(sizeof(sdr_dev_t)))) {
        return NULL;
    }
    if (!(dev->usb = sdr_usb_open(bus, port, SDR_DEV_VID, SDR_DEV_PID))) {
        free(dev);
        return NULL;
    }
    if (!read_sample_type(dev)) {
        sdr_usb_close(dev->usb);
        free(dev);
        fprintf(stderr, "Read sampling type error\n");
        return NULL;
    }
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
#if 1
        dev->data[i] = libusb_dev_mem_alloc(dev->usb->h, SDR_SIZE_BUFF);
#else
        dev->data[i] = (uint8_t *)malloc(SDR_SIZE_BUFF);
#endif
        if (!(dev->transfer[i] = libusb_alloc_transfer(0))) {
            sdr_usb_close(dev->usb);
            free(dev);
            return NULL;
        }
        libusb_fill_bulk_transfer(dev->transfer[i], dev->usb->h, SDR_DEV_EP,
            dev->data[i], SDR_SIZE_BUFF, transfer_cb, dev, TO_TR_LIBUSB);
    }
    gen_LUT();
    
    dev->state = 1;
    dev->rp = dev->wp = 0;
    pthread_create(&dev->thread, NULL, event_handler_thread, dev);
    
    // set thread scheduling real-time
    if (pthread_setschedparam(dev->thread, SCHED_RR, &param)) {
        fprintf(stderr, "set thread scheduling error\n");
    }
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        if ((ret = libusb_submit_transfer(dev->transfer[i]))) {
            fprintf(stderr, "libusb_submit_transfer(%d) error (%d)\n", i, ret);
        }
    }
    return dev;

#endif // WIN32
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
#ifdef WIN32
    dev->state = 0;
    WaitForSingleObject(dev->thread, 10000);
    CloseHandle(dev->thread);
    
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        delete [] dev->buff[i];
    }
    sdr_usb_close(dev->usb);
    delete dev;
#else
    dev->state = 0;
    pthread_join(dev->thread, NULL);
    
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        libusb_cancel_transfer(dev->transfer[i]);
    }
    sdr_usb_close(dev->usb);
    
    for (int i = 0; i < SDR_MAX_BUFF; i++) {
        libusb_free_transfer(dev->transfer[i]);
#if 1
        libusb_dev_mem_free(dev->usb->h, dev->data[i], SDR_SIZE_BUFF);
#else
        free(dev->data[i]);
#endif
    }
    free(dev);
#endif // WIN32
}

// copy digital IF data --------------------------------------------------------
static int copy_data(const uint8_t *data, int ch, int IQ, int8_t *buff)
{
    int size = SDR_SIZE_BUFF;
    
    if (IQ == 0) { // raw
        if (ch != 0) return 0;
        memcpy(buff, data, size);
    }
    else if (IQ == 1) { // 8 bit I sampling
        for (int i = 0; i < size; i += 4) {
            buff[i  ] = LUT[ch][0][data[i  ]];
            buff[i+1] = LUT[ch][0][data[i+1]];
            buff[i+2] = LUT[ch][0][data[i+2]];
            buff[i+3] = LUT[ch][0][data[i+3]];
        }
    }
    else if (IQ == 2) { // 8 bit I/Q sampling
        size *= 2;
        for (int i = 0, j = 0; i < size; i += 8, j += 4) {
            buff[i  ] = LUT[ch][0][data[j  ]];
            buff[i+1] = LUT[ch][1][data[j  ]];
            buff[i+2] = LUT[ch][0][data[j+1]];
            buff[i+3] = LUT[ch][1][data[j+1]];
            buff[i+4] = LUT[ch][0][data[j+2]];
            buff[i+5] = LUT[ch][1][data[j+2]];
            buff[i+6] = LUT[ch][0][data[j+3]];
            buff[i+7] = LUT[ch][1][data[j+3]];
        }
    }
    else if (IQ == 3) { // 16 bit I sampling
        int n = ch / 2 % 2, m = ch % 2;
        size /= 2;
        for (int i = 0, j = ch / 4; i < size; i += 4, j += 8) {
            buff[i  ] = LUT[n][m][data[j  ]];
            buff[i+1] = LUT[n][m][data[j+2]];
            buff[i+2] = LUT[n][m][data[j+4]];
            buff[i+3] = LUT[n][m][data[j+6]];
        }
    }
    return size;
}

// get digital IF data ---------------------------------------------------------
int sdr_dev_data(sdr_dev_t *dev, int8_t **buff, int *n)
{
    uint8_t *data;
    int size = 0;
    
    for (int i = 0; i < dev->max_ch; i++) {
        n[i] = 0;
    }
    while ((data = read_buff(dev))) {
        for (int i = 0; i < dev->max_ch; i++) {
            n[i] += copy_data(data, i, dev->IQ[i], buff[i] + n[i]);
        }
        size += SDR_SIZE_BUFF;
    }
    return size;
}
