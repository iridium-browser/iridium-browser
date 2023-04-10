// Copyright 2012 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_SERVICE_CONSTANTS_H_
#define SYSTEM_API_DBUS_SERVICE_CONSTANTS_H_

#include <stdint.h>  // for uint32_t

// We use relative includes here to make this compatible with both the
// Chromium OS and Chromium environment.
#include "anomaly_detector/dbus-constants.h"
#include "arc-data-snapshotd/dbus-constants.h"
#include "arcvm_data_migrator/dbus-constants.h"
#include "audio/dbus-constants.h"
#include "authpolicy/dbus-constants.h"
#include "biod/dbus-constants.h"
#include "bluetooth/dbus-constants.h"
#include "bootlockbox/dbus-constants.h"
#include "cecservice/dbus-constants.h"
#include "cfm/dbus-constants.h"
#include "chunneld/dbus-constants.h"
#include "cros-disks/dbus-constants.h"
#include "cros_healthd/dbus-constants.h"
#include "cryptohome/dbus-constants.h"
#include "dcad/dbus-constants.h"
#include "debugd/dbus-constants.h"
#include "discod/dbus-constants.h"
#include "dlp/dbus-constants.h"
#include "drivefs/dbus-constants.h"
#include "faced/dbus-constants.h"
#include "featured/dbus-constants.h"
#include "fusebox/dbus-constants.h"
#include "hammerd/dbus-constants.h"
#include "hermes/dbus-constants.h"
#include "hiberman/dbus-constants.h"
#include "hps/dbus-constants.h"
#include "ip_peripheral/dbus-constants.h"
#include "login_manager/dbus-constants.h"
#include "lorgnette/dbus-constants.h"
#include "missive/dbus-constants.h"
#include "modemfwd/dbus-constants.h"
#include "ocr/dbus-constants.h"
#include "oobe_config/dbus-constants.h"
#include "os_install_service/dbus-constants.h"
#include "patchpanel/dbus-constants.h"
#include "permission_broker/dbus-constants.h"
#include "power_manager/dbus-constants.h"
#include "printscanmgr/dbus-constants.h"
#include "privacy_screen/dbus-constants.h"
#include "resource_manager/dbus-constants.h"
#include "rgbkbd/dbus-constants.h"
#include "rmad/dbus-constants.h"
#include "runtime_probe/dbus-constants.h"
#include "seneschal/dbus-constants.h"
#include "shill/dbus-constants.h"
#include "smbfs/dbus-constants.h"
#include "smbprovider/dbus-constants.h"
#include "spaced/dbus-constants.h"
#include "swap_management/dbus-constants.h"
#include "update_engine/dbus-constants.h"
#include "usbguard/dbus-constants.h"
#include "vm_applications/dbus-constants.h"
#include "vm_cicerone/dbus-constants.h"
#include "vm_concierge/dbus-constants.h"
#include "vm_disk_management/dbus-constants.h"
#include "vm_plugin_dispatcher/dbus-constants.h"
#include "vm_sk_forwarding/dbus-constants.h"
#include "wilco_dtc_supportd/dbus-constants.h"

namespace dbus {
const char kDBusInterface[] = "org.freedesktop.DBus";
const char kDBusServiceName[] = "org.freedesktop.DBus";
const char kDBusServicePath[] = "/org/freedesktop/DBus";

// Object Manager interface
const char kDBusObjectManagerInterface[] = "org.freedesktop.DBus.ObjectManager";
// Methods
const char kDBusObjectManagerGetManagedObjects[] = "GetManagedObjects";
// Signals
const char kDBusObjectManagerInterfacesAddedSignal[] = "InterfacesAdded";
const char kDBusObjectManagerInterfacesRemovedSignal[] = "InterfacesRemoved";

// Properties interface
const char kDBusPropertiesInterface[] = "org.freedesktop.DBus.Properties";
// Methods
const char kDBusPropertiesGet[] = "Get";
const char kDBusPropertiesSet[] = "Set";
const char kDBusPropertiesGetAll[] = "GetAll";
// Signals
const char kDBusPropertiesChangedSignal[] = "PropertiesChanged";
}  // namespace dbus

namespace imageburn {
const char kImageBurnServiceName[] = "org.chromium.ImageBurner";
const char kImageBurnServicePath[] = "/org/chromium/ImageBurner";
const char kImageBurnServiceInterface[] = "org.chromium.ImageBurnerInterface";
// Methods
const char kBurnImage[] = "BurnImage";
// Signals
const char kSignalBurnFinishedName[] = "burn_finished";
const char kSignalBurnUpdateName[] = "burn_progress_update";
}  // namespace imageburn

namespace imageloader {
const char kImageLoaderServiceInterface[] = "org.chromium.ImageLoaderInterface";
const char kImageLoaderServiceName[] = "org.chromium.ImageLoader";
const char kImageLoaderServicePath[] = "/org/chromium/ImageLoader";
// Methods
const char kRegisterComponent[] = "RegisterComponent";
const char kLoadComponent[] = "LoadComponent";
const char kLoadComponentAtPath[] = "LoadComponentAtPath";
const char kGetComponentVersion[] = "GetComponentVersion";
const char kRemoveComponent[] = "RemoveComponent";
const char kUnmountComponent[] = "UnmountComponent";
const char kLoadDlcImage[] = "LoadDlcImage";
// Constants
const char kBadResult[] = "";
const char kTerminaComponentName[] = "cros-termina";
const char kSlotNameA[] = "Dlc-A";
const char kSlotNameB[] = "Dlc-B";
}  // namespace imageloader

namespace speech_synthesis {
const char kSpeechSynthesizerInterface[] =
    "org.chromium.SpeechSynthesizerInterface";
const char kSpeechSynthesizerServicePath[] = "/org/chromium/SpeechSynthesizer";
const char kSpeechSynthesizerServiceName[] = "org.chromium.SpeechSynthesizer";
// Methods
const char kSpeak[] = "Speak";
const char kStop[] = "Stop";
const char kIsSpeaking[] = "IsSpeaking";
const char kShutdown[] = "Shutdown";
}  // namespace speech_synthesis

namespace chromium {
const char kChromiumInterface[] = "org.chromium.Chromium";
// Text-to-speech service signals.
const char kTTSReadySignal[] = "TTSReady";
const char kTTSFailedSignal[] = "TTSFailed";
}  // namespace chromium

// Services in the chromeos namespace are owned by Chrome. Different services
// may be instantiated in different Chrome processes.
namespace chromeos {

const char kNetworkProxyServiceName[] = "org.chromium.NetworkProxyService";
const char kNetworkProxyServicePath[] = "/org/chromium/NetworkProxyService";
const char kNetworkProxyServiceInterface[] =
    "org.chromium.NetworkProxyServiceInterface";
const char kNetworkProxyServiceResolveProxyMethod[] = "ResolveProxy";

// Options to override the default behaviour of system-proxy, a local daemon
// which does proxy authentication to a remote web proxy, on behalf of Chrome OS
// system services. The default behaviour is to return the address of
// system-proxy as the first entry in the PAC-style list of resolved proxy only
// if the device policy SystemProxySettings is enabled.
enum SystemProxyOverride {
  // Default behaviour. System-proxy will be appended to the list of returned
  // proxies only if enabled by policy.
  kDefault = 0,
  // System-proxy will be appended to the list of returned proxies only if
  // enabled by policy or feature flag SystemProxyForSystemServices.
  kOptIn = 1,
  // System-proxy will not be added to the list of returned proxies, even if
  // enabled by policy.
  kOptOut = 2,
};

const char kLivenessServiceName[] = "org.chromium.LivenessService";
const char kLivenessServicePath[] = "/org/chromium/LivenessService";
const char kLivenessServiceInterface[] =
    "org.chromium.LivenessServiceInterface";
const char kLivenessServiceCheckLivenessMethod[] = "CheckLiveness";

const char kMetricsEventServiceName[] = "org.chromium.MetricsEventService";
const char kMetricsEventServicePath[] = "/org/chromium/MetricsEventService";
const char kMetricsEventServiceInterface[] =
    "org.chromium.MetricsEventServiceInterface";
const char kMetricsEventServiceChromeEventSignal[] = "ChromeEvent";

const char kComponentUpdaterServiceName[] =
    "org.chromium.ComponentUpdaterService";
const char kComponentUpdaterServicePath[] =
    "/org/chromium/ComponentUpdaterService";
const char kComponentUpdaterServiceInterface[] =
    "org.chromium.ComponentUpdaterService";
const char kComponentUpdaterServiceLoadComponentMethod[] = "LoadComponent";
const char kComponentUpdaterServiceUnloadComponentMethod[] = "UnloadComponent";
const char kComponentUpdaterServiceComponentInstalledSignal[] =
    "ComponentInstalled";

const char kKioskAppServiceName[] = "org.chromium.KioskAppService";
const char kKioskAppServicePath[] = "/org/chromium/KioskAppService";
const char kKioskAppServiceInterface[] =
    "org.chromium.KioskAppServiceInterface";
const char kKioskAppServiceGetRequiredPlatformVersionMethod[] =
    "GetRequiredPlatformVersion";

const char kDisplayServiceName[] = "org.chromium.DisplayService";
const char kDisplayServicePath[] = "/org/chromium/DisplayService";
const char kDisplayServiceInterface[] = "org.chromium.DisplayServiceInterface";
const char kDisplayServiceSetPowerMethod[] = "SetPower";
const char kDisplayServiceSetSoftwareDimmingMethod[] = "SetSoftwareDimming";
const char kDisplayServiceTakeOwnershipMethod[] = "TakeOwnership";
const char kDisplayServiceReleaseOwnershipMethod[] = "ReleaseOwnership";
enum DisplayPowerState {
  DISPLAY_POWER_ALL_ON = 0,
  DISPLAY_POWER_ALL_OFF = 1,
  DISPLAY_POWER_INTERNAL_OFF_EXTERNAL_ON = 2,
  DISPLAY_POWER_INTERNAL_ON_EXTERNAL_OFF = 3,
};

const char kScreenLockServiceName[] = "org.chromium.ScreenLockService";
const char kScreenLockServicePath[] = "/org/chromium/ScreenLockService";
const char kScreenLockServiceInterface[] =
    "org.chromium.ScreenLockServiceInterface";
const char kScreenLockServiceShowLockScreenMethod[] = "ShowLockScreen";

const char kUserAuthenticationServiceName[] =
    "org.chromium.UserAuthenticationService";
const char kUserAuthenticationServicePath[] =
    "/org/chromium/UserAuthenticationService";
const char kUserAuthenticationServiceInterface[] =
    "org.chromium.UserAuthenticationServiceInterface";
const char kUserAuthenticationServiceShowAuthDialogMethod[] = "ShowAuthDialog";
const char kUserAuthenticationServiceShowAuthDialogV2Method[] =
    "ShowAuthDialogV2";
const char kUserAuthenticationServiceCancelMethod[] = "Cancel";
const char kUserAuthenticationServiceIsAuthenticatorAvailableMethod[] =
    "IsAuthenticatorAvailable";

constexpr char kVirtualFileRequestServiceName[] =
    "org.chromium.VirtualFileRequestService";
constexpr char kVirtualFileRequestServicePath[] =
    "/org/chromium/VirtualFileRequestService";
constexpr char kVirtualFileRequestServiceInterface[] =
    "org.chromium.VirtualFileRequestService";
constexpr char kVirtualFileRequestServiceHandleReadRequestMethod[] =
    "HandleReadRequest";
constexpr char kVirtualFileRequestServiceHandleIdReleasedMethod[] =
    "HandleIdReleased";

const char kChromeFeaturesServiceName[] = "org.chromium.ChromeFeaturesService";
const char kChromeFeaturesServicePath[] = "/org/chromium/ChromeFeaturesService";
const char kChromeFeaturesServiceInterface[] =
    "org.chromium.ChromeFeaturesServiceInterface";
const char kChromeFeaturesServiceIsFeatureEnabledMethod[] = "IsFeatureEnabled";
const char kChromeFeaturesServiceGetFeatureParamsMethod[] = "GetFeatureParams";
const char kChromeFeaturesServiceIsCrostiniEnabledMethod[] =
    "IsCrostiniEnabled";
const char kChromeFeaturesServiceIsCryptohomeDistributedModelEnabledMethod[] =
    "IsCryptohomeDistributedModelEnabled";
const char kChromeFeaturesServiceIsCryptohomeUserDataAuthEnabledMethod[] =
    "IsCryptohomeUserDataAuthEnabled";
const char
    kChromeFeaturesServiceIsCryptohomeUserDataAuthKillswitchEnabledMethod[] =
        "IsCryptohomeUserDataAuthKillswitchEnabled";
const char kChromeFeaturesServiceIsPluginVmEnabledMethod[] =
    "IsPluginVmEnabled";
const char kChromeFeaturesServiceIsVmManagementCliAllowedMethod[] =
    "IsVmManagementCliAllowed";
const char kChromeFeaturesServiceIsShillSandboxingEnabledMethod[] =
    "IsShillSandboxingEnabled";
const char kChromeFeaturesServiceIsFsNosymfollowEnabledMethod[] =
    "IsFsNosymfollowEnabled";
const char kChromeFeaturesServiceIsPeripheralDataAccessEnabledMethod[] =
    "IsPeripheralDataAccessEnabled";
const char kChromeFeaturesServiceIsDNSProxyEnabledMethod[] =
    "IsDNSProxyEnabled";

const char kUrlHandlerServiceName[] = "org.chromium.UrlHandlerService";
const char kUrlHandlerServicePath[] = "/org/chromium/UrlHandlerService";
const char kUrlHandlerServiceInterface[] =
    "org.chromium.UrlHandlerServiceInterface";
const char kUrlHandlerServiceOpenUrlMethod[] = "OpenUrl";

const char kPluginVmServiceName[] = "org.chromium.PluginVmService";
const char kPluginVmServicePath[] = "/org/chromium/PluginVmService";
const char kPluginVmServiceInterface[] =
    "org.chromium.PluginVmServiceInterface";
const char kPluginVmServiceGetLicenseDataMethod[] = "GetLicenseData";
const char kPluginVmServiceShowSettingsPage[] = "ShowSettingsPage";
const char kPluginVmServiceGetPermissionsMethod[] = "GetPermissions";
const char kPluginVmServiceGetAppLicenseUserId[] = "GetAppLicenseUserId";

const char kGesturePropertiesServiceName[] =
    "org.chromium.GesturePropertiesService";
const char kGesturePropertiesServicePath[] =
    "/org/chromium/GesturePropertiesService";
const char kGesturePropertiesServiceInterface[] =
    "org.chromium.GesturePropertiesServiceInterface";
const char kGesturePropertiesServiceListDevicesMethod[] = "ListDevices";
const char kGesturePropertiesServiceListPropertiesMethod[] = "ListProperties";
const char kGesturePropertiesServiceGetPropertyMethod[] = "GetProperty";
const char kGesturePropertiesServiceSetPropertyMethod[] = "SetProperty";

const char kPrintersServiceName[] = "org.chromium.PrintersService";
const char kPrintersServicePath[] = "/org/chromium/PrintersService";
const char kPrintersServiceInterface[] =
    "org.chromium.PrintersServiceInterface";
const char kPrintersServicePrintersChangedSignal[] = "PrintersChanged";

constexpr char kMlDecisionServiceName[] = "org.chromium.MlDecisionService";
constexpr char kMlDecisionServicePath[] = "/org/chromium/MlDecisionService";
constexpr char kMlDecisionServiceInterface[] = "org.chromium.MlDecisionService";
constexpr char kMlDecisionServiceShouldDeferScreenDimMethod[] =
    "ShouldDeferScreenDim";

const char kVmPermissionServiceName[] = "org.chromium.VmPermissionService";
const char kVmPermissionServicePath[] = "/org/chromium/VmPermissionService";
const char kVmPermissionServiceInterface[] =
    "org.chromium.VmPermissionServiceInterface";

const char kVmPermissionServiceRegisterVmMethod[] = "RegisterVm";
const char kVmPermissionServiceUnregisterVmMethod[] = "UnregisterVm";
const char kVmPermissionServiceGetPermissionsMethod[] = "GetPermissions";
const char kVmPermissionServiceSetPermissionsMethod[] = "SetPermissions";

constexpr char kChromeReportingServiceInterface[] =
    "org.chromium.ChromeReportingServiceInterface";
constexpr char kChromeReportingServicePath[] =
    "/org/chromium/ChromeReportingService";
constexpr char kChromeReportingServiceName[] =
    "org.chromium.ChromeReportingService";
constexpr char kChromeReportingServiceUploadEncryptedRecordMethod[] =
    "UploadEncryptedRecord";
}  // namespace chromeos

namespace media_perception {

const char kMediaPerceptionServiceName[] = "org.chromium.MediaPerception";
const char kMediaPerceptionServicePath[] = "/org/chromium/MediaPerception";
const char kMediaPerceptionInterface[] = "org.chromium.MediaPerception";

const char kStateFunction[] = "State";
const char kGetDiagnosticsFunction[] = "GetDiagnostics";
const char kDetectionSignal[] = "MediaPerceptionDetection";
const char kBootstrapMojoConnection[] = "BootstrapMojoConnection";

}  // namespace media_perception

namespace modemmanager {
// ModemManager D-Bus service identifiers
const char kModemManagerSMSInterface[] =
    "org.freedesktop.ModemManager.Modem.Gsm.SMS";

// ModemManager function names.
const char kSMSGetFunction[] = "Get";
const char kSMSDeleteFunction[] = "Delete";
const char kSMSListFunction[] = "List";

// ModemManager monitored signals
const char kSMSReceivedSignal[] = "SmsReceived";

// ModemManager1 interfaces and signals
// The canonical source for these constants is:
//   /usr/include/ModemManager/ModemManager-names.h
const char kModemManager1ServiceName[] = "org.freedesktop.ModemManager1";
const char kModemManager1ServicePath[] = "/org/freedesktop/ModemManager1";
const char kModemManager1ModemInterface[] =
    "org.freedesktop.ModemManager1.Modem";
const char kModemManager1MessagingInterface[] =
    "org.freedesktop.ModemManager1.Modem.Messaging";
const char kModemManager1SmsInterface[] = "org.freedesktop.ModemManager1.Sms";
const char kModemManager1SarInterface[] =
    "org.freedesktop.ModemManager1.Modem.Sar";

const char kSMSAddedSignal[] = "Added";
const char kSarEnable[] = "Enable";
const char kSarSetPowerLevel[] = "SetPowerLevel";

}  // namespace modemmanager

namespace mtpd {
const char kMtpdInterface[] = "org.chromium.Mtpd";
const char kMtpdServicePath[] = "/org/chromium/Mtpd";
const char kMtpdServiceName[] = "org.chromium.Mtpd";
const char kMtpdServiceError[] = "org.chromium.Mtpd.Error";

// Methods.
const char kEnumerateStorages[] = "EnumerateStorages";
const char kGetStorageInfo[] = "GetStorageInfo";
const char kGetStorageInfoFromDevice[] = "GetStorageInfoFromDevice";
const char kOpenStorage[] = "OpenStorage";
const char kCloseStorage[] = "CloseStorage";
const char kReadDirectoryEntryIds[] = "ReadDirectoryEntryIds";
const char kGetFileInfo[] = "GetFileInfo";
const char kReadFileChunk[] = "ReadFileChunk";
const char kCopyFileFromLocal[] = "CopyFileFromLocal";
const char kDeleteObject[] = "DeleteObject";
const char kRenameObject[] = "RenameObject";
const char kCreateDirectory[] = "CreateDirectory";

// Signals.
const char kMTPStorageAttached[] = "MTPStorageAttached";
const char kMTPStorageDetached[] = "MTPStorageDetached";

// For FileEntry struct:
const uint32_t kInvalidFileId = 0xffffffff;

// For OpenStorage method:
const char kReadOnlyMode[] = "ro";
const char kReadWriteMode[] = "rw";

// For GetFileInfo() method:
// The id of the root node in a storage, as defined by the PTP/MTP standards.
// Use this when referring to the root node in the context of GetFileInfo().
const uint32_t kRootFileId = 0;
}  // namespace mtpd

namespace system_clock {
const char kSystemClockInterface[] = "org.torproject.tlsdate";
const char kSystemClockServicePath[] = "/org/torproject/tlsdate";
const char kSystemClockServiceName[] = "org.torproject.tlsdate";

// Methods.
const char kSystemClockCanSet[] = "CanSetTime";
const char kSystemClockSet[] = "SetTime";
const char kSystemLastSyncInfo[] = "LastSyncInfo";

// Signals.
const char kSystemClockUpdated[] = "TimeUpdated";
}  // namespace system_clock

namespace feedback {
const char kFeedbackServicePath[] = "/org/chromium/feedback";
const char kFeedbackServiceName[] = "org.chromium.feedback";

// Methods.
const char kSendFeedback[] = "SendFeedback";
}  // namespace feedback

namespace easy_unlock {
const char kEasyUnlockServicePath[] = "/org/chromium/EasyUnlock";
const char kEasyUnlockServiceName[] = "org.chromium.EasyUnlock";
const char kEasyUnlockServiceInterface[] = "org.chromium.EasyUnlock";

// Values supplied as enrcryption type to CreateSecureMessage and
// UnwrapSecureMessage methods.
const char kEncryptionTypeNone[] = "NONE";
const char kEncryptionTypeAES256CBC[] = "AES_256_CBC";

// Values supplied as signature type to CreateSecureMessage and
// UnwrapSecureMessage methods.
const char kSignatureTypeECDSAP256SHA256[] = "ECDSA_P256_SHA256";
const char kSignatureTypeHMACSHA256[] = "HMAC_SHA256";

// Values supplied as key algorithm to WrapPublicKey method.
const char kKeyAlgorithmRSA[] = "RSA";
const char kKeyAlgorithmECDSA[] = "ECDSA";

// Methods
const char kPerformECDHKeyAgreementMethod[] = "PerformECDHKeyAgreement";
const char kWrapPublicKeyMethod[] = "WrapPublicKey";
const char kGenerateEcP256KeyPairMethod[] = "GenerateEcP256KeyPair";
const char kCreateSecureMessageMethod[] = "CreateSecureMessage";
const char kUnwrapSecureMessageMethod[] = "UnwrapSecureMessage";
}  // namespace easy_unlock

namespace cdm_oemcrypto {
const char kCdmFactoryDaemonServiceInterface[] =
    "org.chromium.CdmFactoryDaemon";
const char kCdmFactoryDaemonServiceName[] = "org.chromium.CdmFactoryDaemon";
const char kCdmFactoryDaemonServicePath[] = "/org/chromium/CdmFactoryDaemon";
// Methods
const char kBootstrapCdmFactoryDaemonMojoConnection[] =
    "BootstrapCdmFactoryDaemonMojoConnection";
const char kGetFactoryTransportKeyMaterial[] = "GetFactoryTransportKeyMaterial";
const char kWrapFactoryKeybox[] = "WrapFactoryKeybox";
const char kGetClientInformation[] = "GetClientInformation";
}  // namespace cdm_oemcrypto

namespace midis {
constexpr char kMidisServiceName[] = "org.chromium.Midis";
constexpr char kMidisServicePath[] = "/org/chromium/Midis";
constexpr char kMidisInterfaceName[] = "org.chromium.Midis";
// Methods
constexpr char kBootstrapMojoConnectionMethod[] = "BootstrapMojoConnection";
}  // namespace midis

namespace ml {
constexpr char kMachineLearningServiceName[] = "org.chromium.MachineLearning";
constexpr char kMachineLearningServicePath[] = "/org/chromium/MachineLearning";
constexpr char kMachineLearningInterfaceName[] = "org.chromium.MachineLearning";
// Methods
constexpr char kBootstrapMojoConnectionMethod[] = "BootstrapMojoConnection";
// Token identifying the primordial Mojo pipe passed to BootstrapMojoConnection.
constexpr char kBootstrapMojoConnectionChannelToken[] = "ml-service-bootstrap";

constexpr char kMachineLearningAdaptiveChargingServiceName[] =
    "org.chromium.MachineLearning.AdaptiveCharging";
constexpr char kMachineLearningAdaptiveChargingServicePath[] =
    "/org/chromium/MachineLearning/AdaptiveCharging";
constexpr char kMachineLearningAdaptiveChargingInterfaceName[] =
    "org.chromium.MachineLearning.AdaptiveCharging";
// Methods
constexpr char kRequestAdaptiveChargingDecisionMethod[] =
    "RequestAdaptiveChargingDecision";

}  // namespace ml

namespace federated {
constexpr char kFederatedServiceName[] = "org.chromium.Federated";
constexpr char kFederatedServicePath[] = "/org/chromium/Federated";
constexpr char kFederatedInterfaceName[] = "org.chromium.Federated";
// Methods
constexpr char kBootstrapMojoConnectionMethod[] = "BootstrapMojoConnection";
// Token identifying the primordial Mojo pipe passed to BootstrapMojoConnection.
constexpr char kBootstrapMojoConnectionChannelToken[] =
    "federated-service-bootstrap";
}  // namespace federated

// D-Bus MojoConnectionServiceProvider (go/mojo_connection_service) is a D-Bus
// service provider in Chrome that bootstraps CrOS services' mojo connection.
namespace mojo_connection_service {
constexpr char kMojoConnectionServiceServiceName[] =
    "org.chromium.MojoConnectionService";
constexpr char kMojoConnectionServiceServicePath[] =
    "/org/chromium/MojoConnectionService";
constexpr char kMojoConnectionServiceInterface[] =
    "org.chromium.MojoConnectionService";

// Methods
constexpr char kBootstrapMojoConnectionForIioServiceMethod[] =
    "BootstrapMojoConnectionForIioService";
constexpr char kBootstrapMojoConnectionForSensorClientsMethod[] =
    "BootstrapMojoConnectionForSensorClients";
constexpr char kBootstrapMojoConnectionForRollbackNetworkConfigMethod[] =
    "BootstrapMojoConnectionForRollbackNetworkConfigService";
constexpr char kBootstrapForCrosHealthdInternalServiceFactoryMethod[] =
    "kBootstrapForCrosHealthdInternalServiceFactoryMethod";
}  // namespace mojo_connection_service

namespace virtual_file_provider {
constexpr char kVirtualFileProviderServiceName[] =
    "org.chromium.VirtualFileProvider";
constexpr char kVirtualFileProviderServicePath[] =
    "/org/chromium/VirtualFileProvider";
constexpr char kVirtualFileProviderInterface[] =
    "org.chromium.VirtualFileProvider";
// Methods
constexpr char kGenerateVirtualFileIdMethod[] = "GenerateVirtualFileId";
constexpr char kOpenFileByIdMethod[] = "OpenFileById";
}  // namespace virtual_file_provider

namespace crosdns {
constexpr char kCrosDnsServiceName[] = "org.chromium.CrosDns";
constexpr char kCrosDnsServicePath[] = "/org/chromium/CrosDns";
constexpr char kCrosDnsInterfaceName[] = "org.chromium.CrosDns";
// Methods
constexpr char kSetHostnameIpMappingMethod[] = "SetHostnameIpMapping";
constexpr char kRemoveHostnameIpMappingMethod[] = "RemoveHostnameIpMapping";
}  // namespace crosdns

namespace arc {

constexpr char kArcServiceName[] = "org.chromium.Arc";
constexpr char kArcServicePath[] = "/org/chromium/Arc";
constexpr char kArcInterfaceName[] = "org.chromium.Arc";

// Signal
constexpr char kArcStopped[] = "ArcStopped";

namespace keymaster {
constexpr char kArcKeymasterServiceName[] = "org.chromium.ArcKeymaster";
constexpr char kArcKeymasterServicePath[] = "/org/chromium/ArcKeymaster";
constexpr char kArcKeymasterInterfaceName[] = "org.chromium.ArcKeymaster";
// Methods
constexpr char kBootstrapMojoConnectionMethod[] = "BootstrapMojoConnection";
}  // namespace keymaster

namespace keymint {
constexpr char kArcKeyMintServiceName[] = "org.chromium.ArcKeyMint";
constexpr char kArcKeyMintServicePath[] = "/org/chromium/ArcKeyMint";
constexpr char kArcKeyMintInterfaceName[] = "org.chromium.ArcKeyMint";
// Methods
constexpr char kBootstrapMojoConnectionMethod[] = "BootstrapMojoConnection";
}  // namespace keymint

namespace obb_mounter {
// D-Bus service constants.
constexpr char kArcObbMounterInterface[] =
    "org.chromium.ArcObbMounterInterface";
constexpr char kArcObbMounterServicePath[] = "/org/chromium/ArcObbMounter";
constexpr char kArcObbMounterServiceName[] = "org.chromium.ArcObbMounter";

// Method names.
constexpr char kMountObbMethod[] = "MountObb";
constexpr char kUnmountObbMethod[] = "UnmountObb";
}  // namespace obb_mounter

namespace appfuse {
// D-Bus service constants.
constexpr char kArcAppfuseProviderInterface[] =
    "org.chromium.ArcAppfuseProvider";
constexpr char kArcAppfuseProviderServicePath[] =
    "/org/chromium/ArcAppfuseProvider";
constexpr char kArcAppfuseProviderServiceName[] =
    "org.chromium.ArcAppfuseProvider";

// Method names.
constexpr char kMountMethod[] = "Mount";
constexpr char kUnmountMethod[] = "Unmount";
constexpr char kOpenFileMethod[] = "OpenFile";
}  // namespace appfuse

}  // namespace arc

namespace libvda {
const char kLibvdaServiceInterface[] = "org.chromium.LibvdaService";
const char kLibvdaServiceName[] = "org.chromium.LibvdaService";
const char kLibvdaServicePath[] = "/org/chromium/LibvdaService";

// Method names.
const char kProvideMojoConnectionMethod[] = "ProvideMojoConnection";
}  // namespace libvda

namespace printing {
constexpr char kCupsProxyDaemonName[] = "org.chromium.CupsProxyDaemon";
constexpr char kCupsProxyDaemonPath[] = "/org/chromium/CupsProxyDaemon";
constexpr char kCupsProxyDaemonInterface[] = "org.chromium.CupsProxyDaemon";

// Method names.
constexpr char kBootstrapMojoConnectionMethod[] = "BootstrapMojoConnection";

// Token identifying the primordial Mojo pipe passed to BootstrapMojoConnection.
constexpr char kBootstrapMojoConnectionChannelToken[] =
    "cups-proxy-service-bootstrap";
}  // namespace printing

namespace arc_camera {
constexpr char kArcCameraServiceName[] = "org.chromium.ArcCamera";
constexpr char kArcCameraServicePath[] = "/org/chromium/ArcCamera";
constexpr char kArcCameraServiceInterface[] = "org.chromium.ArcCamera";

// Method names.
constexpr char kStartServiceMethod[] = "StartService";
}  // namespace arc_camera

namespace lock_to_single_user {
const char kLockToSingleUserInterface[] = "org.chromium.LockToSingleUser";
const char kLockToSingleUserServicePath[] = "/org/chromium/LockToSingleUser";
const char kLockToSingleUserServiceName[] = "org.chromium.LockToSingleUser";

const char kNotifyVmStartingMethod[] = "NotifyVmStarting";
}  // namespace lock_to_single_user

#endif  // SYSTEM_API_DBUS_SERVICE_CONSTANTS_H_
