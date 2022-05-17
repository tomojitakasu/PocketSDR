#!/usr/bin/env python3
#
#  generate C hex table from python binary table
#
import sys
sys.path.append('../python')
sys.path.append('.')
import sdr_code

print('static const uint16_t B2AD_G2_init[] = {')
for i in range(len(sdr_code.B2AD_G2_init)):
    if i % 8 == 0:
        print('    ', end='')
    print('0x%04X, ' % (sdr_code.B2AD_G2_init[i]), end='')
    if i % 8 == 7:
        print('')
print('\n};')

print('static const uint16_t B2AP_G2_init[] = {')
for i in range(len(sdr_code.B2AP_G2_init)):
    if i % 8 == 0:
        print('    ', end='')
    print('0x%04X, ' % (sdr_code.B2AP_G2_init[i]), end='')
    if i % 8 == 7:
        print('')
print('\n};')

print('static const uint16_t B2BI_G2_init[] = {')
for i in range(len(sdr_code.B2BI_G2_init)):
    if i % 8 == 0:
        print('    ', end='')
    print('0x%04X, ' % (sdr_code.B2BI_G2_init[i]), end='')
    if i % 8 == 7:
        print('')
print('\n};')

print('static const uint16_t B3I_G2_init[] = {')
for i in range(len(sdr_code.B3I_G2_init)):
    if i % 8 == 0:
        print('    ', end='')
    print('0x%04X, ' % (sdr_code.B3I_G2_init[i]), end='')
    if i % 8 == 7:
        print('')
print('\n};')

print('static const uint16_t I5S_G2_init[] = {')
for i in range(len(sdr_code.I5S_G2_init)):
    if i % 8 == 0:
        print('    ', end='')
    print('0x%04X, ' % (sdr_code.I5S_G2_init[i]), end='')
    if i % 8 == 7:
        print('')
print('\n};')

print('static const uint16_t ISS_G2_init[] = {')
for i in range(len(sdr_code.ISS_G2_init)):
    if i % 8 == 0:
        print('    ', end='')
    print('0x%04X, ' % (sdr_code.ISS_G2_init[i]), end='')
    if i % 8 == 7:
        print('')
print('\n};')

