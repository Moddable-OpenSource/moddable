@ECHO OFF
REM Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
REM 2016 Qualcomm Atheros, Inc.
REM All Rights Reserved
REM Copyright (c) 2018 Qualcomm Technologies, Inc.
REM All rights reserved.
REM Redistribution and use in source and binary forms, with or without modification, are permitted (subject to the limitations in the disclaimer below)
REM provided that the following conditions are met:
REM Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
REM Redistributions in binary form must reproduce the above copyright notice,
REM this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
REM Neither the name of Qualcomm Technologies, Inc. nor the names of its contributors may be used to endorse or promote products derived
REM from this software without specific prior written permission.
REM NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
REM THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
REM BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
REM IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
REM OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
REM LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
REM WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
REM EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

SETLOCAL EnableDelayedExpansion

REM Default CHIPSET_VERSION is v2
IF /I "%CHIPSET_VERSION%" == ""    (SET CHIPSET_VERSION=v2)
IF /I "%CFG_FEATURE_HMI%" == ""         (SET CFG_FEATURE_HMI=true)
IF /I "%CFG_FEATURE_BLE%" == ""         (SET CFG_FEATURE_BLE=true)
IF /I "%CFG_FEATURE_WLAN%" == ""        (SET CFG_FEATURE_WLAN=true)
IF /I "%CFG_FEATURE_WLAN_8021X%" == ""  (SET CFG_FEATURE_WLAN_8021X=false)
IF /I "%CFG_FEATURE_FLASHLOG%" == ""    (SET CFG_FEATURE_FLASHLOG=true)
IF /I "%CFG_FEATURE_LP%" == ""          (SET CFG_FEATURE_LP=true)
IF /I "%CFG_FEATURE_ZIGBEE%" == ""      (SET CFG_FEATURE_ZIGBEE=true)
IF /I "%CFG_FEATURE_COEX%" == ""        (SET CFG_FEATURE_COEX=true)
IF /I "%CFG_FEATURE_FWUP%" == ""        (SET CFG_FEATURE_FWUP=true)
IF /I "%CFG_FEATURE_FS%" == ""          (SET CFG_FEATURE_FS=true)
IF /I "%CFG_FEATURE_SECUREFS%" == ""    (SET CFG_FEATURE_SECUREFS=true)
IF /I "%CFG_FEATURE_THREAD%" == ""      (SET CFG_FEATURE_THREAD=true)
IF /I "%CFG_FEATURE_I2S%" == ""         (SET CFG_FEATURE_I2S=false)
IF /I "%CFG_FEATURE_PERIPHERALS%" == "" (SET CFG_FEATURE_PERIPHERALS=false)
IF /I "%CFG_FEATURE_PLATFORM%" == ""    (SET CFG_FEATURE_PLATFORM=true)
IF /I "%CFG_FEATURE_ECOSYSTEM%" == ""   (SET CFG_FEATURE_ECOSYSTEM=true)
IF /I "%CFG_FEATURE_KPI_DEMO%" == ""    (SET CFG_FEATURE_KPI_DEMO=false)
IF /I "%CFG_FEATURE_JSON%" == ""        (SET CFG_FEATURE_JSON=true)
IF /I "%CFG_FEATURE_NET%" == ""         (SET CFG_FEATURE_NET=true)
IF /I "%CFG_FEATURE_NET_PING%" == ""    (SET CFG_FEATURE_NET_PING=true)
IF /I "%CFG_FEATURE_NET_ROUTE%" == ""   (SET CFG_FEATURE_NET_ROUTE=true)
IF /I "%CFG_FEATURE_NET_TXRX%" == ""    (SET CFG_FEATURE_NET_TXRX=true)
IF /I "%CFG_FEATURE_NET_SSL%" == ""     (SET CFG_FEATURE_NET_SSL=true)
IF /I "%CFG_FEATURE_NET_DHCPV4C%" == "" (SET CFG_FEATURE_NET_DHCPV4C=true)
IF /I "%CFG_FEATURE_NET_DHCPV6C%" == "" (SET CFG_FEATURE_NET_DHCPV6C=true)
IF /I "%CFG_FEATURE_NET_AUTOIP%" == ""  (SET CFG_FEATURE_NET_AUTOIP=true)
IF /I "%CFG_FEATURE_NET_SNTPC%" == ""   (SET CFG_FEATURE_NET_SNTPC=true)
IF /I "%CFG_FEATURE_NET_HTTPS%" == ""   (SET CFG_FEATURE_NET_HTTPS=true)
IF /I "%CFG_FEATURE_NET_HTTPC%" == ""   (SET CFG_FEATURE_NET_HTTPC=true)
IF /I "%CFG_FEATURE_NET_COAP%" == ""    (SET CFG_FEATURE_NET_COAP=true)
IF /I "%CFG_FEATURE_NET_DNSC%" == ""    (SET CFG_FEATURE_NET_DNSC=true)
IF /I "%CFG_FEATURE_NET_DNSS%" == ""    (SET CFG_FEATURE_NET_DNSS=true)
IF /I "%CFG_FEATURE_NET_MDNSS%" == ""   (SET CFG_FEATURE_NET_MDNSS=true)
IF /I "%CFG_FEATURE_NET_DNSSD%" == ""   (SET CFG_FEATURE_NET_DNSSD=true)
IF /I "%CFG_FEATURE_NET_MQTTC%" == ""   (SET CFG_FEATURE_NET_MQTTC=true)
IF /I "%CFG_FEATURE_NET_USER_ACCOUNT%" == ""   (SET CFG_FEATURE_NET_USER_ACCOUNT=true)
IF /I "%CFG_FEATURE_NET_RAW_SOCKET%" == ""     (SET CFG_FEATURE_NET_RAW_SOCKET=true)
IF /I "%CFG_FEATURE_NET_WLAN_BRIDGE%" == ""    (SET CFG_FEATURE_NET_WLAN_BRIDGE=true)
IF /I "%CFG_FEATURE_NET_TCP_KEEPALIVE%" == ""  (SET CFG_FEATURE_NET_TCP_KEEPALIVE=true)

IF /I "%CFG_FEATURE_CRYPTO%" == ""           (SET CFG_FEATURE_CRYPTO=true)
IF /I "%CFG_FEATURE_CRYPTO_UNIT_TEST%" == "" (SET CFG_FEATURE_CRYPTO_UNIT_TEST=true)
IF /I "%CFG_FEATURE_CRYPTO_BASE64%" == ""    (SET CFG_FEATURE_CRYPTO_BASE64=true)
IF /I "%CFG_FEATURE_CRYPTO_PERSISTENT_OBJECT%" == ""   (SET CFG_FEATURE_CRYPTO_PERSISTENT_OBJECT=true)
IF /I "%CFG_FEATURE_CRYPTO_ATTESTATION%" == ""   (SET CFG_FEATURE_CRYPTO_ATTESTATION=true)
IF /I "%CFG_FEATURE_NET_WEBSOCKETC%" == ""   (SET CFG_FEATURE_NET_WEBSOCKETC=true)
IF /I "%CFG_FEATURE_UART_DEMO%" == ""        (SET CFG_FEATURE_UART_DEMO=true)
IF /I "%CFG_FEATURE_CHK_POINT_GPIO_PIN%" == ""   (SET CFG_FEATURE_CHK_POINT_GPIO_PIN=56)
IF /I "%CFG_FEATURE_RAMDUMP%" == ""          (SET CFG_FEATURE_RAMDUMP=false)
IF /I "%CFG_FEATURE_DEC%" == ""              (SET CFG_FEATURE_DEC=true)
IF /I "%CFG_FEATURE_OPUS_DECODER%" == ""     (SET CFG_FEATURE_OPUS_DECODER=false)
IF /I "%CFG_FEATURE_IMAGE_ENCRYPT%" == ""    (SET CFG_FEATURE_IMAGE_ENCRYPT=false)
IF /I "%CFG_ENCRYPT_KEY%" == ""              (SET CFG_ENCRYPT_KEY=0)
                                      
REM Determine the RTOS to build. Default to freertos.
IF /I "%~1" == "" (
   SET RTOS=freertos
) ELSE IF /I "%~1" == "t" (
   SET RTOS=threadx
) ELSE IF /I "%~1" == "f" (
   SET RTOS=freertos
) ELSE IF /I "%~1" == "prepare" (
REM special command, will export devcfg files
   SET RTOS=freertos
) ELSE IF /I "%~1" == "clobber" (
REM special command, will delete devcfg files
   SET RTOS=freertos
) ELSE (
   ECHO Invalid RTOS: %1
   GOTO:Usage
)

REM Validate the chipset variant. Default: 4020
IF /I "%~2" == "" (
   SET CHIPSET_VARIANT=qca4020
) ELSE IF /I "%2" == "4020" (
   SET CHIPSET_VARIANT=qca4020
) ELSE IF /I "%2" == "4024" (
   SET CHIPSET_VARIANT=qca4024
) ELSE IF /I "%2" == "4025" (
   SET CHIPSET_VARIANT=qca4025
) ELSE (
   ECHO Invalid chipset variant: %2%
   GOTO:Usage
)

IF /I "%CHIPSET_VARIANT%" == "qca4020" (
   SET CFG_FEATURE_WLAN=true
) ELSE (
   SET CFG_FEATURE_WLAN=false
   SET CFG_FEATURE_KPI_DEMO=false
)

REM Validate the board variant  -- carrier, dut and CDB. Default: carrier
IF /I "%~3" == "" (
   SET BOARD_VARIANT=carrier
   SET NVM_LIB_VARIANT=C
) ELSE IF /I "%3" == "c" (
   SET BOARD_VARIANT=carrier
   SET NVM_LIB_VARIANT=C
) ELSE IF /I "%3" == "d" (
   SET BOARD_VARIANT=DUT
   SET NVM_LIB_VARIANT=D
) ELSE IF /I "%3" == "cdb" (
   SET BOARD_VARIANT=CDB
   SET NVM_LIB_VARIANT=CDB
) ELSE (
   ECHO Invalid board variant: %3%
   GOTO:Usage
)

REM Validate the chipset revision. Default: 1.2 for v1, 2.0 for v2
IF /I "%~4" == "" (
   IF /I "%CHIPSET_VERSION%" == "v2" (
      SET CHIPSET_REVISION=2p0
   ) ELSE (
      SET CHIPSET_REVISION=1p2
   )
) ELSE IF /I "%4" == "1p2" (
   SET CHIPSET_REVISION=1p2
) ELSE IF /I "%4" == "2p0" (
   SET CHIPSET_REVISION=2p0
) ELSE (
   ECHO Invalid chipset revision: %4%
   GOTO:Usage
)

REM Get the path of the optional custom NVM file.
IF /I "%~5" == "" (
   SET NVM_FILE=
) ELSE (
   IF EXIST "%~5" (
      SET NVM_FILE=%5
   ) ELSE (
      ECHO Invalid NVM file path: %5%
      GOTO:Usage
   )
)

@ECHO ****************************************************************************
@ECHO                      Building QCA402X QCLI Application for %CHIPSET_VERSION% Chipset
@ECHO                      RTOS             %RTOS%
@ECHO                      CHIPSET VARIANT  %CHIPSET_VARIANT%
@ECHO *****************************************************************************

REM Setup the paths for the build
SET Project=Quartz
SET RootDir=..\..\..\..\..
SET SrcDir=..\..\src
SET NvmDir=%RootDir%\quartz\nvm
SET OutDir=output
SET ObjDir=%OutDir%\objs
SET SectoolsDir=%RootDir%\sectools
SET SectoolsQdnDir=%RootDir%\sectools\qdn
SET SectoolsCertsDir=%SectoolsQdnDir%\resources\data_prov_assets\Signing\Local\qc_presigned_certs-key2048_exp257
SET SECBOOT=false
SET LibDir=%RootDir%\lib\cortex-m4IPT\%RTOS%
SET SymFile="%RootDir%\bin\cortex-m4\IOE_ROM_IPT_IMG_ARNNRI_gcc.sym"
SET SymFileUnpatched="%RootDir%\bin\cortex-m4\IOE_ROM_IPT_IMG_ARNNRI_orig_fcns_gcc.sym"
SET ScriptDir=%RootDir%\build\scripts
SET LinkerScriptDir=%RootDir%\build\scripts\linkerScripts
SET LINKFILE="%OutDir%\%Project%.ld"
SET LIBSFILE="%OutDir%\LinkerLibs.txt"
SET ThirdpartyDir=..\..\..\..
SET EcosystemRoot=..\..\..\ecosystem
SET MeshModelsCommonCode=..\..\..\..\qmesh\models\common
SET MeshClientModels=..\..\..\..\qmesh\models\client
SET MeshServerModels=..\..\..\..\qmesh\models\server

REM Prepare Command, copy device config files to export directory
IF /I "%1" == "prepare" (
   SET exitflag=true
   GOTO Prepare
)

REM Clobber command, delete object files and export directory
IF /I "%1" == "clobber" (
   SET exitflag=true
   GOTO Clobber
)

IF /I "%RTOS%" == "threadx" (
   SET Libs="%LibDir%\threadx.lib"
   SET Libs=!Libs! "%LibDir%\mom_patch_table_ARNTRI_qcm.o"
   SET Libs=!Libs! "%LibDir%\fom_patch_table_ARNTRI_qcm.o"
   SET Libs=!Libs! "%LibDir%\som_patch_table_ARNTRI_qcm.o"
) ELSE (
   SET Libs="%LibDir%\free_rtos.lib"
   SET Libs=!Libs! "%LibDir%\mom_patch_table_ARNFRI_qcm.o"
   SET Libs=!Libs! "%LibDir%\fom_patch_table_ARNFRI_qcm.o"
   SET Libs=!Libs! "%LibDir%\som_patch_table_ARNFRI_qcm.o"
)

REM Sources to compile
SET CWallSrcs=
SET CSrcs=sbrk.c
SET CSrcs=%CSrcs% qcli\qcli.c
SET CSrcs=%CSrcs% qcli\qcli_util.c
SET CSrcs=%CSrcs% qcli\pal.c

IF /I "%CFG_FEATURE_THREAD%" == "true" (
   SET CSrcs=!CSrcs! thread\thread_demo.c
)
IF /I "%CFG_FEATURE_ZIGBEE%" == "true" (
   SET CSrcs=!CSrcs! zigbee\zigbee_demo.c
   SET CSrcs=!CSrcs! zigbee\zdp_demo.c
   SET CSrcs=!CSrcs! zigbee\zcl_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_alarms_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_basic_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_colorcontrol_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_custom_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_devicetemp_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_groups_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_identify_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_levelcontrol_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_onoff_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_ota_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_powerconfig_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_scenes_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_time_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_touchlink_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_doorlock_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_wincover_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_thermostat_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_fancontrol_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_tempmeasure_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_occupancy_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_iaszone_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_iasace_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_iaswd_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_ballast_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_illuminance_demo.c
   SET CSrcs=!CSrcs! zigbee\clusters\zcl_relhumid_demo.c
)
SET CSrcs=%CSrcs% spple\spple_demo.c
SET CSrcs=%CSrcs% spple\ota\ble_ota_service.c
SET CSrcs=%CSrcs% hmi\hmi_demo.c
SET CSrcs=%CSrcs% coex\coex_demo.c

IF /I "%CFG_FEATURE_WLAN%" == "true" (
   SET CSrcs=!CSrcs! wifi\util.c
   SET CSrcs=!CSrcs! wifi\wifi_cmd_handler.c
   SET CSrcs=!CSrcs! wifi\wifi_demo.c
)

IF /I "%CFG_FEATURE_UART_DEMO%" == "true" (
   SET CSrcs=!CSrcs! uart\uart.c
   SET CSrcs=!CSrcs! uart\uart_demo.c
)

IF /I "%CFG_FEATURE_KPI_DEMO%" == "true" (
   SET CWallSrcs=!CWallSrcs! kpi\kpi_demo.c
)
SET CWallSrcs=%CWallSrcs% net\netcmd.c
SET CWallSrcs=%CWallSrcs% net\netutils.c
SET CWallSrcs=%CWallSrcs% net\bench_udp.c
SET CWallSrcs=%CWallSrcs% net\bench_tcp.c
SET CWallSrcs=%CWallSrcs% net\bench_raw.c
SET CWallSrcs=%CWallSrcs% net\bench_ssl.c
SET CWallSrcs=%CWallSrcs% net\bench_uapsd.c
SET CWallSrcs=%CWallSrcs% net\bench.c
SET CWallSrcs=%CWallSrcs% net\iperf.c
SET CWallSrcs=%CWallSrcs% net\eth_raw.c
SET CWallSrcs=%CWallSrcs% net\ssl_demo.c
SET CWallSrcs=%CWallSrcs% net\cert_demo.c
SET CWallSrcs=%CWallSrcs% net\httpc_demo.c
SET CWallSrcs=%CWallSrcs% net\coap_demo.c
SET CWallSrcs=%CWallSrcs% net\websocketc_demo.c
SET CWallSrcs=%CWallSrcs% net\mqttc_demo.c
SET CWallSrcs=%CWallSrcs% net\httpsvr\cgi\htmldata.c
SET CWallSrcs=%CWallSrcs% net\httpsvr\cgi\cgi_showintf.c
SET CWallSrcs=%CWallSrcs% net\httpsvr\cgi\cgi_demo.c
SET CSrcs=%CSrcs% ota\ota_demo.c
SET CSrcs=%CSrcs% ota\plugins\ftp\ota_ftp.c
SET CSrcs=%CSrcs% ota\plugins\http\ota_http.c
SET CSrcs=%CSrcs% ota\plugins\zigbee\ota_zigbee.c
SET CSrcs=%CSrcs% ota\plugins\ble\ota_ble.c
IF /I "%CFG_FEATURE_I2S%" == "true" (
   SET CSrcs=!CSrcs! adss\adss_demo.c
   SET CSrcs=!CSrcs! adss\adss.c
   SET CSrcs=!CSrcs! adss\adss_audio_data.c
   SET CSrcs=!CSrcs! adss\adss_ftp.c
   SET CSrcs=!CSrcs! adss\adss_ftp_rec.c
   SET CSrcs=!CSrcs! adss\adss_pcm.c
   SET CSrcs=!CSrcs! adss\adss_mem.c
)
IF /I "%CFG_FEATURE_FLASHLOG%" == "true" (
   SET CSrcs=!CSrcs! flashlog\flashlog_demo.c
)
SET CSrcs=%CSrcs% spi_master\spi_test_demo.c

SET CSrcs=%CSrcs% master_sdcc\master_sdcc_demo.c
SET CSrcs=%CSrcs% master_sdcc\master_sdcc.c
SET CSrcs=%CSrcs% htc_slave\htc_slave_demo.c
SET CSrcs=%CSrcs% lp\lp_demo.c
SET CSrcs=%CSrcs% lp\fom_lp_test.c
SET CSrcs=%CSrcs% lp\som_lp_test.c
SET CSrcs=%CSrcs% lp\mom_lp_test.c
SET CSrcs=%CSrcs% targetif\htc\src\htc.c
SET CSrcs=%CSrcs% targetif\htc\src\htc_events.c
SET CSrcs=%CSrcs% targetif\htc\src\htc_recv.c
SET CSrcs=%CSrcs% targetif\htc\src\htc_send.c
SET CSrcs=%CSrcs% targetif\htc\src\htc_utils.c
SET CSrcs=%CSrcs% targetif\hif\hif.c
SET CSrcs=%CSrcs% targetif\transport\qurt\transport.c
SET CSrcs=%CSrcs% targetif\transport\qurt\sdio\sdio.c
SET CSrcs=%CSrcs% targetif\transport\qurt\spi\spi_hal.c
SET CSrcs=%CSrcs% targetif\app\htc_demo.c
SET CWallSrcs=%CWallSrcs% ecosystem\ecosystem_demo.c
SET CWallSrcs=%CWallSrcs% fs\fs_demo.c
SET CWallSrcs=%CWallSrcs% securefs\securefs_demo.c
SET CWallSrcs=%CWallSrcs% crypto\crypto_demo.c
SET CWallSrcs=%CWallSrcs% crypto\crypto_helper.c
SET CWallSrcs=%CWallSrcs% crypto\persistent_obj_demo.c
SET CWallSrcs=%CWallSrcs% platform\platform_demo.c
REM SET CSrcs=%CSrcs% thread\thread_demo.c
SET CSrcs=%CSrcs% sensors\sensors_demo.c
SET CSrcs=%CSrcs% sensors\sensors.c
SET CSrcs=%CSrcs% sensors\aws_sensors.c

SET CSrcs=%CSrcs% adc\adc_demo.c
SET CSrcs=%CSrcs% adc\adc.c
SET CSrcs=%CSrcs% pwm\pwm_demo.c
SET CSrcs=%CSrcs% pwm\pwm.c
SET CSrcs=%CSrcs% keypad\keypad_demo.c
SET CSrcs=%CSrcs% keypad\keypad.c
SET CSrcs=%CSrcs% peripherals\peripherals_demo.c
SET CWallSrcs=%CWallSrcs% enc\json_demo.c

SET CSrcs=%CSrcs% export\platform_oem.c
SET CSrcs=%CSrcs% export\platform_oem_som.c
SET CSrcs=%CSrcs% export\platform_oem_mom.c
SET CSrcs=%CSrcs% export\DALConfig_devcfg.c
SET CSrcs=%CSrcs% export\DALConfig_fom.c
SET CSrcs=%CSrcs% export\devcfg_devcfg_data.c
SET CSrcs=%CSrcs% export\devcfg_fom_data.c
SET CSrcs=%CSrcs% export\UsrEDL.c

IF /I "%QMESH%"=="true" (
   SET CSrcs=!CSrcs! qmesh\models\client_handler\model_client_menu.c
   SET CSrcs=!CSrcs! qmesh\models\client_handler\generic_onoff_client_handler.c
   SET CSrcs=!CSrcs! qmesh\models\client_handler\generic_power_onoff_client_handler.c
   SET CSrcs=!CSrcs! qmesh\models\client_handler\generic_default_transition_time_client_handler.c
   SET CSrcs=!CSrcs! qmesh\models\client_handler\generic_level_client_handler.c
   SET CSrcs=!CSrcs! qmesh\models\client_handler\generic_power_level_client_handler.c
   SET CSrcs=!CSrcs! qmesh\models\client_handler\light_lightness_client_handler.c
   SET CSrcs=!CSrcs! qmesh\models\client_handler\light_hsl_client_handler.c
   SET CSrcs=!CSrcs! qmesh\models\client_handler\config_client_handler.c
   SET CSrcs=!CSrcs! qmesh\models\client_handler\model_client_event_handler.c
   SET CWallSrcs=!CWallSrcs! qmesh\models\client_handler\model_client_handler_utils.c
   SET CSrcs=!CSrcs! qmesh\gatt_bearer\src\qmesh_ble_gap.c
   SET CSrcs=!CSrcs! qmesh\gatt_bearer\src\qmesh_ble_gatt.c
   SET CSrcs=!CSrcs! qmesh\gatt_bearer\src\qmesh_ble_gatt_client.c
   SET CSrcs=!CSrcs! qmesh\gatt_bearer\src\qmesh_ble_gatt_server.c
   SET CSrcs=!CSrcs! qmesh\qmesh_demo_menu.c
   SET CSrcs=!CSrcs! qmesh\qmesh_demo_composition.c
   SET CSrcs=!CSrcs! qmesh\qmesh_demo_core.c
   SET CSrcs=!CSrcs! qmesh\qmesh_demo_utilities.c
   SET CSrcs=!CSrcs! qmesh\qmesh_demo_nvm_utilities.c
   SET CSrcs=!CSrcs! !MeshClientModels!\generic_onoff_client.c
   SET CSrcs=!CSrcs! !MeshClientModels!\generic_power_onoff_client.c
   SET CSrcs=!CSrcs! !MeshClientModels!\generic_default_transition_time_client.c
   SET CSrcs=!CSrcs! !MeshClientModels!\generic_level_client.c
   SET CSrcs=!CSrcs! !MeshClientModels!\generic_power_level_client.c
   SET CSrcs=!CSrcs! !MeshClientModels!\light_lightness_client.c
   SET CSrcs=!CSrcs! !MeshClientModels!\light_hsl_client.c
   SET CSrcs=!CSrcs! !MeshModelsCommonCode!\qmesh_cache_mgmt.c
   SET CSrcs=!CSrcs! !MeshModelsCommonCode!\qmesh_delay_cache.c
   SET CSrcs=!CSrcs! !MeshModelsCommonCode!\qmesh_model_common.c
   SET CSrcs=!CSrcs! !MeshModelsCommonCode!\qmesh_model_debug.c
   SET CSrcs=!CSrcs! !MeshModelsCommonCode!\qmesh_model_nvm.c
   SET CWallSrcs=!CWallSrcs! !MeshModelsCommonCode!\qmesh_light_utilities.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_generic_default_transition_time_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_generic_poweronoff_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_generic_poweronoff_setup_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_generic_level_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_generic_powerlevel_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_generic_powerlevel_setup_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_generic_onoff_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_light_hsl_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_light_hsl_hue_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_light_hsl_saturation_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_light_hsl_setup_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_light_lightness_handler.c
   SET CSrcs=!CSrcs! !MeshServerModels!\qmesh_light_lightness_setup_handler.c
   SET CWallSrcs=!CWallSrcs! !MeshServerModels!\qmesh_vendor_model_handler.c
   SET CWallSrcs=!CWallSrcs! qmesh\gatt_bearer\src\qmesh_ble_coex.c
)



IF /I "%BOARD_VARIANT%" == "CDB" (
   SET CSrcs=!CSrcs! gpio\gpio.c
   SET CSrcs=!CSrcs! gpio\gpio_demo.c
   SET CSrcs=!CSrcs! spi\spi.c
   SET CSrcs=!CSrcs! spi\spi_demo.c
)

IF /I "%CFG_FEATURE_OPUS_DECODER%" == "true" (
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\src\opus_decoder.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\dec_API.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\celt_decoder.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\init_decoder.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\modes.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\celt.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\CNG.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\PLC.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\entdec.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\stereo_decode_pred.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\src\opus.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\decoder_set_fs.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\stereo_MS_to_LR.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\decode_indices.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\decode_pulses.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\decode_frame.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\decode_pitch.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\resampler.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\tables_other.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\bands.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\mdct.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\pitch.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\celt_lpc.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\vq.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\mathops.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\Quant_bands.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\entcode.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\rate.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\nlsf2a.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\sum_sqr_shift.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\bwexpander.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\bwexpander_32.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\lpc_analysis_filter.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\lpc_inv_pred_gain.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\entenc.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\tables_pitch_lag.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\tables_NLSF_CB_NB_MB.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\NLSF_unpack.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\tables_gain.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\tables_ltp.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\code_signs.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\shell_coder.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\tables_pulses_per_block.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\decode_parameters.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\decode_core.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\resampler_rom.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\resampler_private_up2_HQ.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\resampler_private_IIR_FIR.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\resampler_private_down_FIR.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\resampler_private_AR2.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\kiss_fft.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\cwrs.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\celt\laplace.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\lpc_fit.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\table_LSF_cos.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\tables_NLSF_CB_WB.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\gain_quant.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\NLSF_decode.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\pitch_est_tables.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\log2lin.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\NLSF_stabilize.c
   SET CSrcs=!CSrcs! %ThirdpartyDir%\thirdparty\opus-1.3-rc\silk\sort.c
)


REM Include directories
SET Includes=-I"%RootDir%\include"
SET Includes=%Includes% -I"%RootDir%\include\qapi"
SET Includes=%Includes% -I"%RootDir%\include\bsp"
SET Includes=%Includes% -I"%RootDir%\qmesh\include"
SET Includes=%Includes% -I"%RootDir%\qmesh\platform\inc"
SET Includes=%Includes% -I"%RootDir%\qmesh\platform\qca402x\inc"
SET Includes=%Includes% -I"%RootDir%\qmesh\models\client"
SET Includes=%Includes% -I"%RootDir%\qmesh\models\include"
SET Includes=%Includes% -I"%RootDir%\qmesh\core\inc"

SET Includes=%Includes% -I"%SrcDir%\qcli"
SET Includes=%Includes% -I"%SrcDir%\spple"
SET Includes=%Includes% -I"%SrcDir%\spple\ota"
SET Includes=%Includes% -I"%SrcDir%\hmi"
SET Includes=%Includes% -I"%SrcDir%\coex"
SET Includes=%Includes% -I"%SrcDir%\net"
SET Includes=%Includes% -I"%SrcDir%\ota"
SET Includes=%Includes% -I"%SrcDir%\lp"
SET Includes=%Includes% -I"%SrcDir%\fs"
SET Includes=%Includes% -I"%SrcDir%\wifi"
SET Includes=%Includes% -I"%SrcDir%\net"
SET Includes=%Includes% -I"%SrcDir%\adss"
SET Includes=%Includes% -I"%SrcDir%\htc_slave"
SET Includes=%Includes% -I"%SrcDir%\spi_master"
SET Includes=%Includes% -I"%SrcDir%\master_sdcc"
SET Includes=%Includes% -I"%SrcDir%\targetif\include"
SET Includes=%Includes% -I"%SrcDir%\targetif\htc\include"
SET Includes=%Includes% -I"%SrcDir%\targetif\hif"
SET Includes=%Includes% -I"%SrcDir%\targetif\osal"
SET Includes=%Includes% -I"%SrcDir%\targetif\transport\qurt"
SET Includes=%Includes% -I"%SrcDir%\targetif\transport\qurt\sdio"
SET Includes=%Includes% -I"%SrcDir%\targetif\transport\qurt\spi"
SET Includes=%Includes% -I"%SrcDir%\targetif\app"
SET Includes=%Includes% -I"%SrcDir%\securefs"
SET Includes=%Includes% -I"%SrcDir%\crypto"
SET Includes=%Includes% -I"%SrcDir%\platform"
SET Includes=%Includes% -I"%SrcDir%\zigbee"
SET Includes=%Includes% -I"%SrcDir%\zigbee\clusters"
SET Includes=%Includes% -I"%SrcDir%\thread"
SET Includes=%Includes% -I"%SrcDir%\adc"
SET Includes=%Includes% -I"%SrcDir%\pwm"
SET Includes=%Includes% -I"%SrcDir%\keypad"
SET Includes=%Includes% -I"%SrcDir%\sensors"
SET Includes=%Includes% -I"%SrcDir%\peripherals"
SET Includes=%Includes% -I"%SrcDir%\ecosystem"
SET Includes=%Includes% -I"%SrcDir%\kpi"
SET Includes=%Includes% -I"%SrcDir%\enc"
SET Includes=%Includes% -I"%SrcDir%\qmesh"
SET Includes=%Includes% -I"%SrcDir%\qmesh\mesh"
SET Includes=%Includes% -I"%SrcDir%\qmesh\models\include"
SET Includes=%Includes% -I"%SrcDir%\qmesh\models\client_handler"
SET Includes=%Includes% -I"%SrcDir%\qmesh\gatt_bearer\inc"
SET Includes=%Includes% -I"%SrcDir%\gpio"
SET Includes=%Includes% -I"%SrcDir%\spi"

IF /I "%CFG_FEATURE_OPUS_DECODER%" == "true" (
   SET Includes=%Includes% -I"%RootDir%\thirdparty\opus-1.3-rc\celt" -I"%RootDir%\thirdparty\opus-1.3-rc\include" -I"%RootDir%\thirdparty\opus-1.3-rc\silk"
)

IF /I "%CFG_FEATURE_FLASHLOG%" == "true" (
   SET Includes=!Includes! -I"%SrcDir%\flashlog"
)



IF /I "%CFG_FEATURE_UART_DEMO%" == "true" (
   SET Includes=!Includes! -I"%SrcDir%\uart"
)




IF /I "%Ecosystem%" == "awsiot" (
   GOTO:awsiot
) ELSE (
   GOTO:skip_awsiot
)
:awsiot
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\external_libs\jsmn\jsmn.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_json_utils.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_mqtt_client.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_mqtt_client_common_internal.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_mqtt_client_connect.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_mqtt_client_publish.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_mqtt_client_subscribe.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_mqtt_client_unsubscribe.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_mqtt_client_yield.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_shadow.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_shadow_actions.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_shadow_json.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_shadow_records.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_jobs_interface.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_jobs_json.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_jobs_topics.c
   SET CSrcs=%CSrcs% %ThirdpartyDir%\thirdparty\aws\awsiot\src\aws_iot_jobs_types.c   
   SET CSrcs=%CSrcs% %EcosystemRoot%\aws\port\aws_timer.c
   SET CSrcs=%CSrcs% %EcosystemRoot%\aws\port\network_qca4020_wrapper.c
   SET CSrcs=%CSrcs% ecosystem\aws\shadow_sample\shadow_sample.c
   SET CSrcs=%CSrcs% ecosystem\aws\jobs_sample\jobs_sample.c

   SET Includes=%Includes% -I"%SrcDir%\ecosystem\aws\shadow_sample"
   SET Includes=%Includes% -I"%SrcDir%\ecosystem\aws\jobs_sample"   
   SET Includes=%Includes% -I"%RootDir%\quartz\ecosystem\aws\port\include"
   SET Includes=%Includes% -I"%RootDir%\thirdparty\aws\awsiot\include"
   SET Includes=%Includes% -I"%RootDir%\thirdparty\aws\awsiot\external_libs\jsmn"
:skip_awsiot

IF /I "%Ecosystem%" == "azure" (
   GOTO:azure
) ELSE (
   GOTO:skip_azure
)
:azure
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\base64.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\buffer.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\connection_string_parser.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\constbuffer.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\consolelogger.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\crt_abstractions.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\constmap.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\doublylinkedlist.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\gballoc.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\gb_stdio.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\gb_time.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\gb_rand.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\hmac.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\hmacsha256.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\http_proxy_io.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\xio.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\singlylinkedlist.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\map.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\sastoken.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\sha1.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\sha224.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\sha384-512.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\strings.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\string_tokenizer.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\urlencode.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\usha.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\vector.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\xlogging.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\c-utility\src\optionhandler.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\src\mqtt_client.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\src\mqtt_codec.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\umqtt\src\mqtt_message.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\agenttypesystem.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\codefirst.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\commanddecoder.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\datamarshaller.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\datapublisher.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\dataserializer.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\iotdevice.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\jsondecoder.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\jsonencoder.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\multitree.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\schema.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\schemalib.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\schemaserializer.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\parson\parson.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\serializer\src\methodreturn.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\iothub_client\src\version.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\iothub_client\src\iothub_message.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\iothub_client\src\iothub_client_ll.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\iothub_client\src\iothub_client.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\iothub_client\src\iothubtransport.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\iothub_client\src\iothubtransport_mqtt_common.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\iothub_client\src\iothubtransportmqtt.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\iothub_client\src\iothub_client_authorization.c
SET CThirdPartySrcs=%CThirdPartySrcs% %ThirdpartyDir%\thirdparty\azure\iothub_client\src\iothub_client_retry_control.c

SET CThirdPartySrcs=%CThirdPartySrcs% ecosystem\azure\devicetwin_simplesample.c
SET CThirdPartySrcs=%CThirdPartySrcs% ecosystem\azure\simplesample_mqtt.c
SET CSrcs=%CSrcs% %EcosystemRoot%\azure\port\lock_qca402x.c
SET CSrcs=%CSrcs% %EcosystemRoot%\azure\port\agenttime_qca402x.c
SET CSrcs=%CSrcs% %EcosystemRoot%\azure\port\platform_qca402x.c
SET CSrcs=%CSrcs% %EcosystemRoot%\azure\port\threadapi_qca402x.c
SET CSrcs=%CSrcs% %EcosystemRoot%\azure\port\tickcounter_qca402x.c
SET CSrcs=%CSrcs% %EcosystemRoot%\azure\port\tlsio_qca402x.c
SET CSrcs=%CSrcs% %EcosystemRoot%\azure\port\certs.c

SET Includes=%Includes% -I"%RootDir%\quartz\ecosystem\azure\inc"
SET Includes=%Includes% -I"%RootDir%\thirdparty\azure\umqtt\inc"
SET Includes=%Includes% -I"%RootDir%\thirdparty\azure\umqtt\c-utility\inc"
SET Includes=%Includes% -I"%RootDir%\thirdparty\azure\serializer\inc"
SET Includes=%Includes% -I"%RootDir%\thirdparty\azure\parson"
SET Includes=%Includes% -I"%RootDir%\thirdparty\azure\iothub_client\inc"
SET Includes=%Includes% -I"%RootDir%\quartz\ecosystem\azure\port"


:skip_azure



REM External objects and libraries
SET Libs=%Libs% "%LibDir%\core.lib"
SET Libs=%Libs% "%LibDir%\qurt.lib"
SET Libs=%Libs% "%LibDir%\quartzplatform.lib"
SET Libs=%Libs% "%LibDir%\quartzplatform_xip.lib"
SET Libs=%Libs% "%LibDir%\WLAN.lib"


IF /I "%CFG_FEATURE_WLAN%" == "true" (
    SET Libs=!Libs! "%LibDir%\WLAN_QAPI.lib"
    SET Libs=!Libs! "%LibDir%\CUST_IPSTACK_INICHE.lib"
    SET Libs=!Libs! "%LibDir%\wlan_lib_common_xip.lib"
    SET Libs=!Libs! "%LibDir%\cust_wlan_lib.lib"
    IF /I "%CFG_FEATURE_WLAN_8021X%" == "true" (
        SET Libs=!Libs! "%LibDir%\wlan_supplicant_8021x_xip.lib"
        SET Libs=!Libs! "%LibDir%\wlan_supplicant_8021x_ram.lib"
    )
)

SET Libs=%Libs% "%LibDir%\WLAN_PROFILER.lib"
SET Libs=%Libs% "%LibDir%\net.lib"
SET Libs=%Libs% "%LibDir%\net_ram.lib"
SET Libs=%Libs% "%LibDir%\dhcpv6c.lib"
SET Libs=%Libs% "%LibDir%\sntpc.lib"
SET Libs=%Libs% "%LibDir%\dnssrvr.lib"
SET Libs=%Libs% "%LibDir%\sharkssl.lib"
SET Libs=%Libs% "%LibDir%\csr.lib"
SET Libs=%Libs% "%LibDir%\cryptolib.lib"
SET Libs=%Libs% "%LibDir%\httpsvr.lib"
SET Libs=%Libs% "%LibDir%\httpc.lib"
SET Libs=%Libs% "%LibDir%\coap.lib"
SET Libs=%Libs% "%LibDir%\websocket.lib"
SET Libs=%Libs% "%LibDir%\mqttc.lib"
SET Libs=%Libs% "%LibDir%\vfs.lib"
SET Libs=%Libs% "%LibDir%\userpass.lib"
SET Libs=%Libs% "%LibDir%\i2s.lib"
SET Libs=%Libs% "%LibDir%\master_sdcc.lib"
SET Libs=%Libs% "%LibDir%\fwup.lib"
SET Libs=%Libs% "%LibDir%\fwup_engine.lib"
SET Libs=%Libs% "%LibDir%\qapi_ed25519.lib"
SET Libs=%Libs% "%LibDir%\qapi_securefs.lib"
SET Libs=%Libs% "%LibDir%\pka_port.lib"
SET Libs=%Libs% "%LibDir%\fs_helper.lib"
SET Libs=%Libs% "%LibDir%\quartz_crypto_qapi.lib"
SET Libs=%Libs% "%LibDir%\zigbee.lib"
SET Libs=%Libs% "%LibDir%\quartz_zigbee.lib"
SET Libs=%Libs% "%LibDir%\thread.lib"
SET Libs=%Libs% "%LibDir%\qapi_thread.lib"
SET Libs=%Libs% "%LibDir%\BLUETOPIA_SERVICES.lib"
SET Libs=%Libs% "%LibDir%\BLUETOPIA_QAPI_SERVICES.lib"
SET Libs=%Libs% "%LibDir%\mdns.lib"
SET Libs=%Libs% "%LibDir%\dnssd.lib"
SET Libs=%Libs% "%LibDir%\otp_tlv.lib"
SET Libs=%Libs% "%LibDir%\base64.lib"
SET Libs=%Libs% "%LibDir%\PERSIST_M4.lib"
SET Libs=%Libs% "%LibDir%\json.lib"
SET Libs=%Libs% "%LibDir%\json_qapi.lib"
SET Libs=%Libs% "%LibDir%\master_sdcc.lib"
SET Libs=%Libs% "%LibDir%\nichestack.lib"
SET Libs=%Libs% "%LibDir%\EDLManager.lib"
SET Libs=%Libs% "%LibDir%\tlv_transport.lib"
SET Libs=%Libs% "%LibDir%\crypto_port.lib"
SET Libs=%Libs% "%LibDir%\tee_master.lib"
SET Libs=%Libs% "%LibDir%\dnsclient.lib"
SET Libs=%Libs% "%LibDir%\securefs.lib"
SET Libs=%Libs% "%LibDir%\securefs_port.lib"
SET Libs=%Libs% "%LibDir%\v2core.lib"
REM Place all v2 patches here
SET PatchObjs=!PatchObjs! "%LibDir%\patch.lib"

IF /I "%QMESH%"=="true" (
   SET Libs=!Libs! "%LibDir%\qmesh.lib"
)

REM Setup the build variables
SET Compiler=arm-none-eabi-gcc
SET ObjCopy=arm-none-eabi-objcopy
SET Archiver=arm-none-eabi-ar
SET Linker=arm-none-eabi-ld
SET ARM_OBJDUMP=arm-none-eabi-objdump

SET COpts=-c -g -mcpu=cortex-m4 -mthumb -fno-short-enums -fno-exceptions -O2 -Os -ffunction-sections  -Wall

SET Defines=
IF /I "%CFG_FEATURE_BLE%" == "true" (SET Defines=!Defines! "-D CONFIG_SPPLE_DEMO")
IF /I "%CFG_FEATURE_HMI%" == "true" (SET Defines=!Defines! "-D CONFIG_HMI_DEMO")
IF /I "%CFG_FEATURE_WLAN%" == "true" (
  SET Defines=!Defines! "-D CONFIG_WIFI_DEMO"
  SET Defines=!Defines! "-D WLAN_DEBUG"
  SET Defines=!Defines! "-D ENABLE_P2P_MODE"
  IF /I "%CFG_FEATURE_WLAN_8021X%" == "true" (
    SET Defines=!Defines! "-D CONFIG_WLAN_8021X"
  )
)
IF /I "%CFG_FEATURE_COEX%" == "true" (SET Defines=!Defines! "-D CONFIG_COEX_DEMO")
IF /I "%CFG_FEATURE_FWUP%" == "true" (SET Defines=!Defines! "-D CONFIG_FWUP_DEMO")
IF /I "%CFG_FEATURE_I2S%" == "true"  (SET Defines=!Defines! "-D CONFIG_ADSS_DEMO")
IF /I "%CFG_FEATURE_LP%" == "true"   (SET Defines=!Defines! "-D CONFIG_LP_DEMO")
IF /I "%CFG_FEATURE_FS%" == "true"   (SET Defines=!Defines! "-D CONFIG_FS_DEMO")
IF /I "%CFG_FEATURE_SECUREFS%" == "true" (SET Defines=!Defines! "-D CONFIG_SECUREFS_DEMO")
IF /I "%CFG_FEATURE_ZIGBEE%" == "true"   (SET Defines=!Defines! "-D CONFIG_ZIGBEE_DEMO")
IF /I "%CFG_FEATURE_THREAD%" == "true"   (SET Defines=!Defines! "-D CONFIG_THREAD_DEMO")
IF /I "%CFG_FEATURE_PERIPHERALS%" == "true" (SET Defines=!Defines! "-D CONFIG_PERIPHERALS_DEMO")
IF /I "%CFG_FEATURE_FLASHLOG%" == "true" (SET Defines=!Defines! "-D CONFIG_FLASHLOG_DEMO")
IF /I "%CFG_FEATURE_PLATFORM%" == "true"  (SET Defines=!Defines! "-D CONFIG_PLATFORM_DEMO")
IF /I "%CFG_FEATURE_ECOSYSTEM%" == "true" (SET Defines=!Defines! "-D CONFIG_ECOSYSTEM_DEMO")
IF /I "%CFG_FEATURE_KPI_DEMO%" == "true" (SET Defines=!Defines! "-D CONFIG_KPI_DEMO")
IF /I "%CFG_FEATURE_UART_DEMO%" == "true" (SET Defines=!Defines! "-D CONFIG_UART_DEMO")
IF /I "%CFG_FEATURE_JSON%" == "true" (SET Defines=!Defines! "-D CONFIG_JSON_DEMO")
IF /I "%CFG_FEATURE_DEC%" == "true" (SET Defines=!Defines! "-D QCOM_DECODER")
IF /I "%CFG_FEATURE_OPUS_DECODER%" == "true" (SET Defines=!Defines! "-D CFG_OPUS_DECODER")

SET Defines=!Defines! "-D HTC_SYNC"
SET Defines=!Defines! "-D DEBUG"
SET Defines=!Defines! -DCFG_FEATURE_CHK_POINT_GPIO_PIN=%CFG_FEATURE_CHK_POINT_GPIO_PIN%

REM ----- Network Services Demos -----
REM Global switch:

IF /I "%CFG_FEATURE_NET%" == "true" (SET Defines=!Defines! "-D CONFIG_NET_DEMO")
IF /I "%CFG_FEATURE_NET_PING%" == "true"     (SET Defines=!Defines! "-D CONFIG_NET_PING_DEMO")
IF /I "%CFG_FEATURE_NET_ROUTE%" == "true"    (SET Defines=!Defines! "-D CONFIG_NET_ROUTE_DEMO")
IF /I "%CFG_FEATURE_NET_TXRX%" == "true"     (SET Defines=!Defines! "-D CONFIG_NET_TXRX_DEMO")
IF /I "%CFG_FEATURE_NET_SSL%" == "true"      (SET Defines=!Defines! "-D CONFIG_NET_SSL_DEMO")
IF /I "%CFG_FEATURE_NET_DHCPV4C%" == "true"  (SET Defines=!Defines! "-D CONFIG_NET_DHCPV4C_DEMO")
IF /I "%CFG_FEATURE_NET_DHCPV6C%" == "true"  (SET Defines=!Defines! "-D CONFIG_NET_DHCPV6C_DEMO")
IF /I "%CFG_FEATURE_NET_AUTOIP%" == "true"   (SET Defines=!Defines! "-D CONFIG_NET_AUTOIP_DEMO")
IF /I "%CFG_FEATURE_NET_SNTPC%" == "true"    (SET Defines=!Defines! "-D CONFIG_NET_SNTPC_DEMO")
IF /I "%CFG_FEATURE_NET_HTTPS%" == "true"    (SET Defines=!Defines! "-D CONFIG_NET_HTTPS_DEMO")
IF /I "%CFG_FEATURE_NET_COAP%" == "true"     (SET Defines=!Defines! "-D CONFIG_NET_COAP_DEMO")
IF /I "%CFG_FEATURE_NET_HTTPS%" == "true"    (SET Defines=!Defines! "-D CONFIG_NET_HTTPC_DEMO")
IF /I "%CFG_FEATURE_NET_DNSC%" == "true"     (SET Defines=!Defines! "-D CONFIG_NET_DNSC_DEMO")
IF /I "%CFG_FEATURE_NET_DNSS%" == "true"     (SET Defines=!Defines! "-D CONFIG_NET_DNSS_DEMO")
IF /I "%CFG_FEATURE_NET_MDNSS%" == "true"    (SET Defines=!Defines! "-D CONFIG_NET_MDNSS_DEMO")
IF /I "%CFG_FEATURE_NET_DNSSD%" == "true"    (SET Defines=!Defines! "-D CONFIG_NET_DNSSD_DEMO")
IF /I "%CFG_FEATURE_NET_MQTTC%" == "true"    (SET Defines=!Defines! "-D CONFIG_NET_MQTTC_DEMO")
IF /I "%CFG_FEATURE_NET_USER_ACCOUNT%" == "true"    (SET Defines=!Defines! "-D CONFIG_NET_USER_ACCOUNT_DEMO")
IF /I "%CFG_FEATURE_NET_RAW_SOCKET%" == "true"      (SET Defines=!Defines! "-D CONFIG_NET_RAW_SOCKET_DEMO")
IF /I "%CFG_FEATURE_NET_WLAN_BRIDGE%" == "true"   (SET Defines=!Defines! "-D CONFIG_NET_WLAN_BRIDGE_DEMO")
IF /I "%CFG_FEATURE_NET_TCP_KEEPALIVE%" == "true"   (SET Defines=!Defines! "-D CONFIG_NET_TCP_KEEPALIVE_DEMO")
IF /I "%CFG_FEATURE_NET_WEBSOCKETC%" == "true"    (SET Defines=!Defines! "-D CONFIG_NET_WEBSOCKETC_DEMO")
IF /I "%BOARD_VARIANT%" == "CDB" (SET Defines=!Defines! "-D CONFIG_CDB_PLATFORM")

REM ----- Crypto Demos -----
REM Global switch:
IF /I "%CFG_FEATURE_CRYPTO%" == "true" (SET Defines=!Defines! "-D CONFIG_CRYPTO_DEMO")

REM Specific crypto demos. Comment to disable a demo:
IF /I "%CFG_FEATURE_CRYPTO_UNIT_TEST%" == "true"      (SET Defines=!Defines! "-D CONFIG_CRYPTO_UNIT_TEST_DEMO")
IF /I "%CFG_FEATURE_CRYPTO_BASE64%" == "true"  (SET Defines=!Defines! "-D CONFIG_CRYPTO_BASE64_DEMO")
IF /I "%CFG_FEATURE_CRYPTO_PERSISTENT_OBJECT%" == "true" (SET Defines=!Defines! "-D CONFIG_CRYPTO_PERSISTENT_OBJECT_DEMO")
IF /I "%CFG_FEATURE_CRYPTO_ATTESTATION%" == "true"  (SET Defines=!Defines! "-D CONFIG_CRYPTO_ATTESTATION_DEMO")

IF /I "%QMESH%"=="true" (
  SET Defines=!Defines! "-D CONFIG_QMESH_DEMO"
  SET Defines=!Defines! "-D PLATFORM_QUARTZ" "-D PLATFORM_MULTITHREAD_SUPPORT"
  SET Defines=!Defines! "-D ENABLE_PROVISIONING"
)

IF /I "%CFG_FEATURE_COEX%"=="true" (
  SET Defines=!Defines! "-D CONFIG_QMESH_COEX_DEMO"
  SET Defines=!Defines! "-D ENABLE_THROUGHPUT_TESTING"
)

IF /I "%I2S_REG_TEST%" == "1" (
    SET Defines=!Defines! "-D I2S_REG_TEST"
)

IF /I "%CFG_FEATURE_RAMDUMP%"=="true" (
   SET Defines=!Defines! "-D CFG_FEATURE_RAMDUMP"
   SET Libs=!Libs! "%LibDir%\ramdump.lib"
)

SET Defines=%Defines% "-D V2"
SET Defines=%Defines% "-D qurt_mutex_init(x)=qurt_mutex_create(x)"
SET Defines=%Defines% "-D qurt_mutex_destroy(x)=qurt_mutex_delete(x)"
SET Defines=%Defines% "-D qurt_signal_init(x)=qurt_signal_create(x)"
SET Defines=%Defines% "-D qurt_signal_destroy(x)=qurt_signal_delete(x)"
SET Defines=%Defines% "-D qurt_mutex_destroy(x)=qurt_mutex_delete(x)"
SET Defines=%Defines% "-D FEATURE_QUARTZ_V2"


IF /I "%ENABLE_DBGCALL%"=="1" (
   SET Includes=%Includes% -I"%RootDir%\quartz\sys\dbgcall\include"
   SET Defines=!Defines! "-D ENABLE_DBGCALL"
   SET Libs=!Libs! "%LibDir%\dbgcall.lib"
   SET Libs=!Libs! "%LibDir%\swintr.lib"
)

IF /I "%Ecosystem%" == "awsiot" (
   SET Defines=!Defines! "-D AWS_IOT"
)

IF /I "%Ecosystem%" == "azure" (
   echo Building Azure IOT SDK
   SET Defines=!Defines! "-D AZURE_IOT"
   SET Defines=!Defines! "-D DONT_USE_UPLOADTOBLOB"
   SET Defines=!Defines! "-D NO_LOGGING"
)

IF /I "%CFG_FEATURE_OPUS_DECODER%" == "true" (
   SET Defines=!Defines! "-D OPUS_BUILD"
   SET Defines=!Defines! "-D VAR_ARRAYS"
   SET Defines=!Defines! "-D FIXED_POINT"
)

SET ExtraLibs=

SET CFlags=%COpts% %Defines% %Includes% -D_WANT_IO_C99_FORMATS

SET LDFlags=-eSBL_Entry -no-wchar-size-warning --no-warn-mismatch -R"%SymFile%" -R"%SymFileUnpatched%" -T"%LINKFILE%" -Map="%OutDir%\%Project%.map" -n --gc-sections

IF /I "%Ecosystem%" == "awsiot" (
   echo "Building for AWS"
   SET ExtraLibs=!ExtraLibs! -L"%NEWLIBPATH%" -L"%TOOLLIBPATH%" -lc -L "%GNULIBPATH%" -lgcc
) ELSE IF /I "%Ecosystem%" == "azure" (
   SET LDFlags=!LDFlags! -L"%NEWLIBPATH%" -L"%TOOLLIBPATH%" -u tlsio_template_interface_description -u tlsio_template_get_interface_description
   SET ExtraLibs=!ExtraLibs! -u _scanf_float -u _printf_float -lc -lnosys -lgcc -lm
   SET CThirdPartyOpts=-c -g -mcpu=cortex-m4 -mthumb -fno-short-enums -fno-exceptions -ffunction-sections -w -O2
   SET CThirdPartyFlags=!CThirdPartyOpts! !Defines! !Includes! -D_WANT_IO_C99_FORMATS
)

REM Clean the output directory. Note the delay is to give Windows an opportunity to remove the old directory tree
RMDIR /s/q "%OutDir%" >NUL 2>&1
TIMEOUT /t 1          >NUL 2>&1
RMDIR /s/q "4020" >NUL 2>&1
MKDIR "%OutDir%"
MKDIR "%ObjDir%"

REM Clean up EDL files.
DEL /F "%SrcDir%\export\UsrEDL.c" >NUL 2>&1

SET Libs=!Libs! "%LibDir%\%CHIPSET_VARIANT%_%CHIPSET_REVISION%_%NVM_LIB_VARIANT%_NVM.lib"

IF "%NVM_FILE%" == "" (
   SET NVM_FILE=%NvmDir%\config\%CHIPSET_REVISION:~0,1%.%CHIPSET_REVISION:~2,1%\%CHIPSET_VARIANT:~3,4%\%BOARD_VARIANT%\%CHIPSET_VARIANT%_%CHIPSET_REVISION%.nvm
)

python %NvmDir%\tool\NVM2C.py -o %SrcDir%\export\UsrEDL.c -i %NVM_FILE%

if errorlevel 1 goto EndOfFile

:Prepare

ECHO Exporting Device config files....

MKDIR %SrcDir%\export >NUL 2>&1

robocopy %RootDir%\quartz\platform\export\ %SrcDir%\export platform_oem.h /XO >NUL 2>&1
robocopy %RootDir%\quartz\platform\export\ %SrcDir%\export platform_oem.c /XO >NUL 2>&1
robocopy %RootDir%\quartz\platform\export\ %SrcDir%\export platform_oem_som.c /XO >NUL 2>&1
robocopy %RootDir%\quartz\platform\export\ %SrcDir%\export platform_oem_mom.c /XO >NUL 2>&1

IF /I "%RTOS%" == "threadx" (
   robocopy %RootDir%\build\tools\devcfg\threadx\ %SrcDir%\export *.* /XO >NUL 2>&1
) else (
   robocopy %RootDir%\build\tools\devcfg\freertos\ %SrcDir%\export *.* /XO >NUL 2>&1
)

IF /I "%exitflag%" == "true" ( GOTO EndOfFile )

:Propgen
ECHO GENERATING DEVCFG....
DEL /F %SrcDir%\export\DALConfig_devcfg.c >NUL 2>&1
DEL /F %SrcDir%\export\devcfg_devcfg_data.c >NUL 2>&1
DEL /F %SrcDir%\export\DALConfig_fom.c >NUL 2>&1
DEL /F %SrcDir%\export\devcfg_fom_data.c >NUL 2>&1

IF /I "%BOARD_VARIANT%" == "CDB" (
REM CDB board uses "CDB" as BOARD_VARIANT. CDB board and carrier board use different DevCfg XML files.
   python %RootDir%/build/tools/devcfg/propgen.py --XmlFile=%SrcDir%/export/DevCfg_master_devcfg_out_cdb.xml --DirName=%SrcDir%/export --ConfigFile=%SrcDir%/export/DALConfig_devcfg.c --DevcfgDataFile=%SrcDir%/export/devcfg_devcfg_data.c --ConfigType=%CHIPSET_VARIANT%_devcfg_xml
   python %RootDir%/build/tools/devcfg/propgen.py --XmlFile=%SrcDir%/export/DevCfg_master_fom_out_cdb.xml --DirName=%SrcDir%/export --ConfigFile=%SrcDir%/export/DALConfig_fom.c --DevcfgDataFile=%SrcDir%/export/devcfg_fom_data.c --ConfigType=%CHIPSET_VARIANT%_fom_xml
) ELSE (
   python %RootDir%/build/tools/devcfg/propgen.py --XmlFile=%SrcDir%/export/DevCfg_master_devcfg_out.xml --DirName=%SrcDir%/export --ConfigFile=%SrcDir%/export/DALConfig_devcfg.c --DevcfgDataFile=%SrcDir%/export/devcfg_devcfg_data.c --ConfigType=%CHIPSET_VARIANT%_devcfg_xml
   python %RootDir%/build/tools/devcfg/propgen.py --XmlFile=%SrcDir%/export/DevCfg_master_fom_out.xml --DirName=%SrcDir%/export --ConfigFile=%SrcDir%/export/DALConfig_fom.c --DevcfgDataFile=%SrcDir%/export/devcfg_fom_data.c --ConfigType=%CHIPSET_VARIANT%_fom_xml
)

REM Compile the source
FOR %%F IN (%CSrcs%) DO (
   ECHO Building %%F
   "%Compiler%" %CFlags% "-D __FILENAME__=\"%%~nF.c\"" -o"%ObjDir%\%%~nF.o" "%SrcDir%\%%F"
   if errorlevel 1 goto EndOfFile
   SET AppObjs=!AppObjs! "%ObjDir%\%%~nF.o"
)

FOR %%F IN (%CWallSrcs%) DO (
   ECHO Building %%F
   "%Compiler%" %CFlags% -Werror "-D __FILENAME__=\"%%~nF.c\"" -o"%ObjDir%\%%~nF.o" "%SrcDir%\%%F"
   if errorlevel 1 goto EndOfFile
   SET AppObjs=!AppObjs! "%ObjDir%\%%~nF.o"
)

REM Compile thirdparty source
FOR %%F IN (%CThirdPartySrcs%) DO (
   ECHO Building %%F
   "%Compiler%" %CThirdPartyFlags% "-D __FILENAME__=\"%%~nF.c\"" -o"%ObjDir%\%%~nF.o" "%SrcDir%\%%F"
   if errorlevel 1 goto EndOfFile
   SET AppObjs=!AppObjs! "%ObjDir%\%%~nF.o"
)

REM Generate the file with the libraries to link
IF EXIST %LIBSFILE% del %LIBSFILE%
IF EXIST %LINKFILE% del %LINKFILE%

FOR %%F IN (%Libs%) DO (
   ECHO %%F >>%LIBSFILE%
)

FOR %%F IN (%AppObjs%) DO (
   ECHO %%F >>%LIBSFILE%
)

FOR %%F IN (%PatchObjs%) DO (
   ECHO %%F >>%LIBSFILE%
)

REM Convert paths to unix style
python fixpaths.py %LIBSFILE%

echo Generating Linker Scripts...
REM Update application PlacementFile
python %LinkerScriptDir%\CreateAppPlacementFile.py %RootDir%\bin\cortex-m4\%RTOS%\sys.placement %RootDir%\bin\cortex-m4\%RTOS%\cust.placement app.config app.placement 2>dbg.CreateApp
if %errorlevel% == 1 (
echo Failed to update application placement file. Check dbg.CreateApp for detail...
goto EndOfFile
)

REM Create a Quartz.ld linker script
python %LinkerScriptDir%\MakeLinkerScript.py %RootDir%\bin\cortex-m4\%RTOS%\DefaultTemplateLinkerScript.ld app.placement %LIBSFILE% > %LINKFILE% 2>dbg.Make
if %errorlevel% == 1 (
echo Failed to create linker script file. Check dbg.Make for detail...
goto EndOfFile
)


ECHO Linking...
REM link the image
"%Linker%" %LDFlags% --start-group @%LIBSFILE%  --end-group %ExtraLibs%  -o"%OutDir%\%Project%_nocompact.elf"

REM Run the diag compaction script to generate the final ELF
python %ScriptDir%\diagMsgCompact.py %OutDir%\%Project%.elf %RootDir%\bin\cortex-m4\diag_msg_QCLI_demo.strdb %OutDir%\%Project%_nocompact.elf %RootDir%/bin/cortex-m4/diag_msg.pkl Final > dictLog

if %errorlevel% == 1 goto EndOfFile

IF /I "%CFG_FEATURE_IMAGE_ENCRYPT%" == "true" (
ECHO Encrypting...
ECHO Using key %CFG_ENCRYPT_KEY%

python %SCRIPTDIR%\elf_segment_encryption\elf_encrpyt.py  -p output\%PROJECT%.elf -k %CFG_ENCRYPT_KEY% -a %SectoolsDir%\bin\WIN\openSSL.exe -c %SCRIPTDIR%\elf_segment_encryption\config.json -o output\%PROJECT%.elf

)

REM Hash
ECHO Hashing...
python %SCRIPTDIR%\createxbl.py -f%OUTDIR%\%PROJECT%.elf -a32 -o%OUTDIR%\%PROJECT%_HASHED.elf
if /I "%SECBOOT%" == "true" (
IF /I "%BOARD_VARIANT%" == "CDB" (
    python %SectoolsQdnDir%\sectools.py iot -p 4020 -g m4 -i %OUTDIR%\%PROJECT%.elf -k %SectoolsCertsDir%\qpsa_rootca.key -c %SectoolsCertsDir%\qpsa_rootca.cer --cfg_oem_id=0xffff --cfg_model_id=0x0000 -o . -s
    python %SectoolsQdnDir%\sectools.py iot -p 4020 -g m0 -i %RootDir%\bin\cortex-m0\threadx\ioe_ram_m0_threadx_ipt.mbn -k %SectoolsCertsDir%\qpsa_rootca.key -c %SectoolsCertsDir%\qpsa_rootca.cer --cfg_oem_id=0xffff --cfg_model_id=0x0000 -o . -s
    python %SectoolsQdnDir%\sectools.py iot -p 4020 -g kf -i %RootDir%\bin\wlan\wlan_fw_img.bin -k %SectoolsCertsDir%\qpsa_rootca.key -c %SectoolsCertsDir%\qpsa_rootca.cer --cfg_oem_id=0xffff --cfg_model_id=0x0000 -o . -s
) ELSE (
   python %SectoolsDir%\sectools.py secimage -i %OUTDIR%\%PROJECT%.elf -c 4020_secimage.xml -sa -g m4 -o .
   python %SectoolsDir%\sectools.py secimage -i %RootDir%\bin\cortex-m0\threadx\ioe_ram_m0_threadx_ipt.mbn -c 4020_secimage.xml -sa -g m0 -o .
   python %SectoolsDir%\sectools.py secimage -i %RootDir%\bin\wlan\wlan_fw_img.bin -c 4020_secimage.xml -sa -g kf -o .
)
)
GOTO:EndOfFile

:Clobber
ECHO Deleting all temporary files ...
RMDIR /s/q "%OutDir%" >NUL 2>&1
RMDIR /s/q %SrcDir%\export >NUL 2>&1
GOTO:EndOfFile

:Usage
ECHO.
ECHO Usage: build.bat [RTOS (FreeRTOS = f, ThreadX = t)] [chipset variant (4020, 4024, 4025)] [board variant (Carrier = c, DUT = d, CDB = cdb)] [chipset revision (1.2 = 1p2, 2.0 = 2p0)] [NVM file]
GOTO:EndOfFile

:EndOfFile
