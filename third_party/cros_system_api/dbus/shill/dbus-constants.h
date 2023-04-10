// Copyright 2015 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_SHILL_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_SHILL_DBUS_CONSTANTS_H_

// TODO(benchan): Reorganize shill constants and remove deprecated ones.
namespace shill {
// Flimflam D-Bus service identifiers.
const char kFlimflamManagerInterface[] = "org.chromium.flimflam.Manager";
const char kFlimflamServiceName[] = "org.chromium.flimflam";
const char kFlimflamServicePath[] = "/";  // crosbug.com/20135
const char kFlimflamServiceInterface[] = "org.chromium.flimflam.Service";
const char kFlimflamIPConfigInterface[] = "org.chromium.flimflam.IPConfig";
const char kFlimflamDeviceInterface[] = "org.chromium.flimflam.Device";
const char kFlimflamProfileInterface[] = "org.chromium.flimflam.Profile";
const char kFlimflamThirdPartyVpnInterface[] =
    "org.chromium.flimflam.ThirdPartyVpn";

// Common function names.
const char kGetPropertiesFunction[] = "GetProperties";
const char kSetPropertyFunction[] = "SetProperty";
const char kClearPropertyFunction[] = "ClearProperty";

// Manager function names.
const char kConfigureServiceFunction[] = "ConfigureService";
const char kConfigureServiceForProfileFunction[] = "ConfigureServiceForProfile";
const char kConnectToBestServicesFunction[] = "ConnectToBestServices";
const char kScanAndConnectToBestServicesFunction[] =
    "ScanAndConnectToBestServices";
const char kCreateConnectivityReportFunction[] = "CreateConnectivityReport";
const char kDisableTechnologyFunction[] = "DisableTechnology";
const char kEnableTechnologyFunction[] = "EnableTechnology";
const char kFindMatchingServiceFunction[] = "FindMatchingService";
const char kGetNetworksForGeolocation[] = "GetNetworksForGeolocation";
const char kGetServiceFunction[] = "GetService";
const char kSetLOHSEnabledFunction[] = "SetLOHSEnabled";
const char kRequestScanFunction[] = "RequestScan";
const char kSetNetworkThrottlingFunction[] = "SetNetworkThrottlingStatus";
const char kSetDNSProxyDOHProvidersFunction[] = "SetDNSProxyDOHProviders";
const char kAddPasspointCredentialsFunction[] = "AddPasspointCredentials";
const char kRemovePasspointCredentialsFunction[] = "RemovePasspointCredentials";
const char kSetTetheringEnabledFunction[] = "SetTetheringEnabled";
const char kCheckTetheringReadinessFunction[] = "CheckTetheringReadiness";

// Service function names.
const char kClearPropertiesFunction[] = "ClearProperties";
const char kCompleteCellularActivationFunction[] = "CompleteCellularActivation";
const char kConnectFunction[] = "Connect";
const char kDisconnectFunction[] = "Disconnect";
const char kGetLoadableProfileEntriesFunction[] = "GetLoadableProfileEntries";
const char kGetWiFiPassphraseFunction[] = "GetWiFiPassphrase";
const char kGetEapPassphraseFunction[] = "GetEapPassphrase";
const char kRemoveServiceFunction[] = "Remove";
const char kRequestPortalDetectionFunction[] = "RequestPortalDetection";
const char kRequestTrafficCountersFunction[] = "RequestTrafficCounters";
const char kResetTrafficCountersFunction[] = "ResetTrafficCounters";
const char kSetPropertiesFunction[] = "SetProperties";

// IPConfig function names.
const char kRemoveConfigFunction[] = "Remove";

// Device function names.
const char kChangePinFunction[] = "ChangePin";
const char kEnterPinFunction[] = "EnterPin";
const char kRegisterFunction[] = "Register";
const char kRequirePinFunction[] = "RequirePin";
const char kResetFunction[] = "Reset";
const char kSetUsbEthernetMacAddressSourceFunction[] =
    "SetUsbEthernetMacAddressSource";
const char kUnblockPinFunction[] = "UnblockPin";

// Profile function names.
const char kDeleteEntryFunction[] = "DeleteEntry";
const char kGetEntryFunction[] = "GetEntry";

// ThirdPartyVpn function names.
const char kOnPacketReceivedFunction[] = "OnPacketReceived";
const char kOnPlatformMessageFunction[] = "OnPlatformMessage";
const char kSetParametersFunction[] = "SetParameters";
const char kSendPacketFunction[] = "SendPacket";
const char kUpdateConnectionStateFunction[] = "UpdateConnectionState";

// Manager property names.
const char kActiveProfileProperty[] = "ActiveProfile";
const char kAlwaysOnVpnPackageProperty[] = "AlwaysOnVpnPackage";
const char kAvailableTechnologiesProperty[] = "AvailableTechnologies";
const char kClaimedDevicesProperty[] = "ClaimedDevices";
const char kConnectedTechnologiesProperty[] = "ConnectedTechnologies";
const char kConnectionStateProperty[] = "ConnectionState";
const char kDefaultServiceProperty[] = "DefaultService";
const char kDefaultTechnologyProperty[] = "DefaultTechnology";
const char kDevicesProperty[] = "Devices";
const char kDhcpPropertyHostnameProperty[] = "DHCPProperty.Hostname";
const char kDisableWiFiVHTProperty[] = "DisableWiFiVHT";
const char kDNSProxyDOHProvidersProperty[] = "DNSProxyDOHProviders";
const char kEnabledTechnologiesProperty[] = "EnabledTechnologies";
const char kLOHSConfigProperty[] = "LOHSConfig";
const char kPortalFallbackHttpUrlsProperty[] = "PortalFallbackHttpUrls";
const char kPortalFallbackHttpsUrlsProperty[] = "PortalFallbackHttpsUrls";
const char kPortalHttpUrlProperty[] = "PortalHttpUrl";
const char kPortalHttpsUrlProperty[] = "PortalHttpsUrl";
const char kProfilesProperty[] = "Profiles";
const char kServiceCompleteListProperty[] = "ServiceCompleteList";
const char kServiceWatchListProperty[] = "ServiceWatchList";
const char kServicesProperty[] = "Services";  // Also used for Profile.
const char kSupportedVPNTypesProperty[] = "SupportedVPNTypes";
const char kTetheringAllowedProperty[] = "TetheringAllowed";
const char kTetheringCapabilitiesProperty[] = "TetheringCapabilities";
const char kTetheringConfigProperty[] = "TetheringConfig";
const char kTetheringStatusProperty[] = "TetheringStatus";
const char kUninitializedTechnologiesProperty[] = "UninitializedTechnologies";
const char kWakeOnLanEnabledProperty[] = "WakeOnLanEnabled";
const char kWifiGlobalFTEnabledProperty[] = "WiFi.GlobalFTEnabled";
const char kWifiScanAllowRoamProperty[] = "WiFi.ScanAllowRoam";

// Manager and DefaultProfile property names (the Manager properties that are
// persisted by a DefaultProfile; these are always read-only for
// DefaultProfile).
const char kArpGatewayProperty[] = "ArpGateway";
const char kCheckPortalListProperty[] = "CheckPortalList";
const char kIgnoredDNSSearchPathsProperty[] = "IgnoredDNSSearchPaths";
const char kNoAutoConnectTechnologiesProperty[] = "NoAutoConnectTechnologies";
const char kProhibitedTechnologiesProperty[] = "ProhibitedTechnologies";

// Base Service property names.
const char kAutoConnectProperty[] = "AutoConnect";
const char kCheckPortalProperty[] = "CheckPortal";
const char kConnectableProperty[] = "Connectable";
const char kDeviceProperty[] = "Device";
const char kDiagnosticsDisconnectsProperty[] = "Diagnostics.Disconnects";
const char kDiagnosticsMisconnectsProperty[] = "Diagnostics.Misconnects";
const char kDnsAutoFallbackProperty[] = "DNSAutoFallback";
const char kEapRemoteCertificationProperty[] = "EAP.RemoteCertification";
const char kErrorDetailsProperty[] = "ErrorDetails";
const char kErrorProperty[] = "Error";
const char kGuidProperty[] = "GUID";
const char kIPConfigProperty[] = "IPConfig";
const char kIsConnectedProperty[] = "IsConnected";
const char kLinkMonitorDisableProperty[] = "LinkMonitorDisable";
const char kManagedCredentialsProperty[] = "ManagedCredentials";
const char kMeteredProperty[] = "Metered";
const char kNameProperty[] = "Name";  // Also used for Device and Profile.
const char kPassphraseRequiredProperty[] = "PassphraseRequired";
const char kPortalDetectionFailedPhaseProperty[] = "PortalDetectionFailedPhase";
const char kPortalDetectionFailedStatusProperty[] =
    "PortalDetectionFailedStatus";
const char kPortalDetectionFailedStatusCodeProperty[] =
    "PortalDetectionFailedStatusCode";
const char kPreviousErrorProperty[] = "PreviousError";
const char kPreviousErrorSerialNumberProperty[] = "PreviousErrorSerialNumber";
const char kPriorityProperty[] = "Priority";
const char kProbeUrlProperty[] = "ProbeUrl";
const char kProfileProperty[] = "Profile";
const char kProxyConfigProperty[] = "ProxyConfig";
const char kSaveCredentialsProperty[] = "SaveCredentials";
const char kSavedIPConfigProperty[] = "SavedIPConfig";
const char kSignalStrengthProperty[] = "Strength";
const char kStateProperty[] = "State";
const char kStaticIPConfigProperty[] = "StaticIPConfig";
const char kTrafficCounterResetTimeProperty[] = "TrafficCounterResetTime";
const char kTypeProperty[] = "Type";
const char kUIDataProperty[] = "UIData";
const char kVisibleProperty[] = "Visible";
const char kONCSourceProperty[] = "ONCSource";
const char kUplinkSpeedPropertyKbps[] = "UplinkSpeedKbps";
const char kDownlinkSpeedPropertyKbps[] = "DownlinkSpeedKbps";

// Cellular Service property names.
const char kActivationStateProperty[] = "Cellular.ActivationState";
const char kActivationTypeProperty[] = "Cellular.ActivationType";
const char kCellularAllowRoamingProperty[] = "Cellular.AllowRoaming";
const char kCellularApnProperty[] = "Cellular.APN";
const char kCellularLastConnectedDefaultApnProperty[] =
    "Cellular.LastConnectedDefaultApnProperty";
const char kCellularLastConnectedAttachApnProperty[] =
    "Cellular.LastConnectedAttachApnProperty";
const char kCellularLastGoodApnProperty[] = "Cellular.LastGoodAPN";
const char kCellularLastAttachApnProperty[] = "Cellular.LastAttachAPN";
const char kCellularPPPPasswordProperty[] = "Cellular.PPP.Password";
const char kCellularPPPUsernameProperty[] = "Cellular.PPP.Username";
const char kCellularUserApnListProperty[] = "Cellular.UserAPNList";
const char kNetworkTechnologyProperty[] = "Cellular.NetworkTechnology";
const char kOutOfCreditsProperty[] = "Cellular.OutOfCredits";
const char kPaymentPortalProperty[] = "Cellular.Olp";
const char kRoamingStateProperty[] = "Cellular.RoamingState";
const char kServingOperatorProperty[] = "Cellular.ServingOperator";
const char kTechnologyFamilyProperty[] = "Cellular.Family";
const char kUsageURLProperty[] = "Cellular.UsageUrl";

// EAP Service/Passpoint credentials property names.
const char kEapAnonymousIdentityProperty[] = "EAP.AnonymousIdentity";
const char kEapCaCertIdProperty[] = "EAP.CACertID";
const char kEapCaCertPemProperty[] = "EAP.CACertPEM";
const char kEapCaCertProperty[] = "EAP.CACert";
const char kEapCertIdProperty[] = "EAP.CertID";
const char kEapDomainSuffixMatchProperty[] = "EAP.DomainSuffixMatch";
const char kEapIdentityProperty[] = "EAP.Identity";
const char kEapKeyIdProperty[] = "EAP.KeyID";
const char kEapKeyMgmtProperty[] = "EAP.KeyMgmt";
const char kEapMethodProperty[] = "EAP.EAP";
const char kEapPasswordProperty[] = "EAP.Password";
const char kEapPhase2AuthProperty[] = "EAP.InnerEAP";
const char kEapPinProperty[] = "EAP.PIN";
const char kEapSubjectAlternativeNameMatchProperty[] =
    "EAP.SubjectAlternativeNameMatch";
const char kEapSubjectMatchProperty[] = "EAP.SubjectMatch";
const char kEapTLSVersionMaxProperty[] = "EAP.TLSVersionMax";
const char kEapUseLoginPasswordProperty[] = "EAP.UseLoginPassword";
const char kEapUseProactiveKeyCachingProperty[] = "EAP.UseProactiveKeyCaching";
const char kEapUseSystemCasProperty[] = "EAP.UseSystemCAs";
const char kEapSubjectAlternativeNameMatchTypeProperty[] = "Type";
const char kEapSubjectAlternativeNameMatchValueProperty[] = "Value";
const char kPasspointFQDNProperty[] = "Passpoint.FQDN";
const char kPasspointProvisioningSourceProperty[] =
    "Passpoint.ProvisioningSource";
const char kPasspointMatchTypeProperty[] = "Passpoint.MatchType";
const char kPasspointIDProperty[] = "Passpoint.ID";

// WiFi Service property names.
const char kCountryProperty[] = "Country";
const char kModeProperty[] = "Mode";
const char kPassphraseProperty[] = "Passphrase";
const char kSecurityClassProperty[] = "SecurityClass";
const char kSecurityProperty[] = "Security";
const char kSSIDProperty[] = "SSID";
const char kWifiBSsid[] = "WiFi.BSSID";
const char kWifiFrequencyListProperty[] = "WiFi.FrequencyList";
const char kWifiFrequency[] = "WiFi.Frequency";
const char kWifiHexSsid[] = "WiFi.HexSSID";
const char kWifiHiddenSsid[] = "WiFi.HiddenSSID";
const char kWifiPhyMode[] = "WiFi.PhyMode";
static constexpr char kWifiRandomMACPolicy[] = "WiFi.RandomMACPolicy";
const char kWifiRekeyInProgressProperty[] = "WiFi.RekeyInProgress";
const char kWifiRoamStateProperty[] = "WiFi.RoamState";
const char kWifiVendorInformationProperty[] = "WiFi.VendorInformation";
const char kWifiSignalStrengthRssiProperty[] = "WiFi.SignalStrengthRssi";
const char kWifiBSSIDAllowlist[] = "WiFi.BSSIDAllowlist";

// Base VPN Service property names.
const char kHostProperty[] = "Host";
const char kPhysicalTechnologyProperty[] = "PhysicalTechnology";
const char kProviderProperty[] = "Provider";
const char kProviderHostProperty[] = "Provider.Host";
const char kProviderTypeProperty[] = "Provider.Type";

// ArcVpn Service property names.
const char kArcVpnTunnelChromeProperty[] = "ArcVpn.TunnelChrome";

// IKEv2 VPN Service property names.
const char kIKEv2AuthenticationTypeProperty[] = "IKEv2.AuthenticationType";
const char kIKEv2CaCertPemProperty[] = "IKEv2.CACertPEM";
const char kIKEv2ClientCertIdProperty[] = "IKEv2.ClientCertID";
const char kIKEv2ClientCertSlotProperty[] = "IKEv2.ClientCertSlot";
const char kIKEv2LocalIdentityProperty[] = "IKEv2.LocalIdentity";
const char kIKEv2PskProperty[] = "IKEv2.PSK";
const char kIKEv2RemoteIdentityProperty[] = "IKEv2.RemoteIdentity";

// Values used in IKEv2.AuthenticationType.
const char kIKEv2AuthenticationTypePSK[] = "PSK";
const char kIKEv2AuthenticationTypeEAP[] = "EAP";
const char kIKEv2AuthenticationTypeCert[] = "Cert";

// L2TPIPsec Service property names.
const char kL2TPIPsecCaCertPemProperty[] = "L2TPIPsec.CACertPEM";
const char kL2TPIPsecClientCertIdProperty[] = "L2TPIPsec.ClientCertID";
const char kL2TPIPsecClientCertSlotProperty[] = "L2TPIPsec.ClientCertSlot";
const char kL2TPIPsecLcpEchoDisabledProperty[] = "L2TPIPsec.LCPEchoDisabled";
const char kL2TPIPsecPasswordProperty[] = "L2TPIPsec.Password";
const char kL2TPIPsecPinProperty[] = "L2TPIPsec.PIN";
const char kL2TPIPsecPskProperty[] = "L2TPIPsec.PSK";
const char kL2TPIPsecPskRequiredProperty[] = "L2TPIPsec.PSKRequired";
const char kL2TPIPsecTunnelGroupProperty[] = "L2TPIPsec.TunnelGroup";
const char kL2TPIPsecUseLoginPasswordProperty[] = "L2TPIPsec.UseLoginPassword";
const char kL2TPIPsecUserProperty[] = "L2TPIPsec.User";
const char kL2TPIPsecXauthPasswordProperty[] = "L2TPIPsec.XauthPassword";
const char kL2TPIPsecXauthUserProperty[] = "L2TPIPsec.XauthUser";

// OpenVPN Service property names.
const char kOpenVPNAuthNoCacheProperty[] = "OpenVPN.AuthNoCache";
const char kOpenVPNAuthProperty[] = "OpenVPN.Auth";
const char kOpenVPNAuthRetryProperty[] = "OpenVPN.AuthRetry";
const char kOpenVPNAuthUserPassProperty[] = "OpenVPN.AuthUserPass";
const char kOpenVPNCaCertPemProperty[] = "OpenVPN.CACertPEM";
const char kOpenVPNCipherProperty[] = "OpenVPN.Cipher";
const char kOpenVPNClientCertIdProperty[] = "OpenVPN.Pkcs11.ID";
const char kOpenVPNCompLZOProperty[] = "OpenVPN.CompLZO";
const char kOpenVPNCompNoAdaptProperty[] = "OpenVPN.CompNoAdapt";
const char kOpenVPNCompressProperty[] = "OpenVPN.Compress";
const char kOpenVPNExtraCertPemProperty[] = "OpenVPN.ExtraCertPEM";
const char kOpenVPNExtraHostsProperty[] = "OpenVPN.ExtraHosts";
const char kOpenVPNIgnoreDefaultRouteProperty[] = "OpenVPN.IgnoreDefaultRoute";
const char kOpenVPNKeyDirectionProperty[] = "OpenVPN.KeyDirection";
const char kOpenVPNNsCertTypeProperty[] = "OpenVPN.NsCertType";
const char kOpenVPNOTPProperty[] = "OpenVPN.OTP";
const char kOpenVPNPasswordProperty[] = "OpenVPN.Password";
const char kOpenVPNPinProperty[] = "OpenVPN.Pkcs11.PIN";
const char kOpenVPNPingExitProperty[] = "OpenVPN.PingExit";
const char kOpenVPNPingProperty[] = "OpenVPN.Ping";
const char kOpenVPNPingRestartProperty[] = "OpenVPN.PingRestart";
const char kOpenVPNPortProperty[] = "OpenVPN.Port";
const char kOpenVPNProtoProperty[] = "OpenVPN.Proto";
const char kOpenVPNPushPeerInfoProperty[] = "OpenVPN.PushPeerInfo";
const char kOpenVPNRemoteCertEKUProperty[] = "OpenVPN.RemoteCertEKU";
const char kOpenVPNRemoteCertKUProperty[] = "OpenVPN.RemoteCertKU";
const char kOpenVPNRemoteCertTLSProperty[] = "OpenVPN.RemoteCertTLS";
const char kOpenVPNRenegSecProperty[] = "OpenVPN.RenegSec";
const char kOpenVPNServerPollTimeoutProperty[] = "OpenVPN.ServerPollTimeout";
const char kOpenVPNShaperProperty[] = "OpenVPN.Shaper";
const char kOpenVPNStaticChallengeProperty[] = "OpenVPN.StaticChallenge";
const char kOpenVPNTLSAuthContentsProperty[] = "OpenVPN.TLSAuthContents";
const char kOpenVPNTLSAuthProperty[] = "OpenVPN.TLSAuth";
const char kOpenVPNTLSRemoteProperty[] = "OpenVPN.TLSRemote";
const char kOpenVPNTLSVersionMinProperty[] = "OpenVPN.TLSVersionMin";
const char kOpenVPNTokenProperty[] = "OpenVPN.Token";
const char kOpenVPNUserProperty[] = "OpenVPN.User";
const char kOpenVPNVerbProperty[] = "OpenVPN.Verb";
const char kOpenVPNVerifyHashProperty[] = "OpenVPN.VerifyHash";
const char kOpenVPNVerifyX509NameProperty[] = "OpenVPN.VerifyX509Name";
const char kOpenVPNVerifyX509TypeProperty[] = "OpenVPN.VerifyX509Type";
const char kVPNMTUProperty[] = "VPN.MTU";

// ThirdPartyVpn Service property names.
const char kConfigurationNameProperty[] = "ConfigurationName";
const char kExtensionNameProperty[] = "ExtensionName";
const char kObjectPathSuffixProperty[] = "ObjectPathSuffix";

// WireGuard Service property names.
const char kWireGuardIPAddress[] = "WireGuard.IPAddress";
const char kWireGuardPrivateKey[] = "WireGuard.PrivateKey";
const char kWireGuardPublicKey[] = "WireGuard.PublicKey";
const char kWireGuardPeers[] = "WireGuard.Peers";
// Property names of a peer in "WireGuard.Peers"
const char kWireGuardPeerPublicKey[] = "PublicKey";
const char kWireGuardPeerPresharedKey[] = "PresharedKey";
const char kWireGuardPeerEndpoint[] = "Endpoint";
const char kWireGuardPeerAllowedIPs[] = "AllowedIPs";
const char kWireGuardPeerPersistentKeepalive[] = "PersistentKeepalive";

// IPConfig property names.
// kAddressProperty: Defined below for Device.
const char kBroadcastProperty[] = "Broadcast";
const char kDomainNameProperty[] = "DomainName";
const char kExcludedRoutesProperty[] = "ExcludedRoutes";
const char kGatewayProperty[] = "Gateway";
const char kIncludedRoutesProperty[] = "IncludedRoutes";
const char kLeaseDurationSecondsProperty[] = "LeaseDurationSeconds";
const char kMethodProperty[] = "Method";
const char kMtuProperty[] = "Mtu";
const char kNameServersProperty[] = "NameServers";
const char kPeerAddressProperty[] = "PeerAddress";
const char kPrefixlenProperty[] = "Prefixlen";
const char kSearchDomainsProperty[] = "SearchDomains";
const char kVendorEncapsulatedOptionsProperty[] = "VendorEncapsulatedOptions";
const char kWebProxyAutoDiscoveryUrlProperty[] = "WebProxyAutoDiscoveryUrl";
const char kiSNSOptionDataProperty[] = "iSNSOptionData";  // For RFC 4174.

// Passpoint credentials property names.
// EAP properties are defined above for EAP service.
const char kPasspointCredentialsFQDNProperty[] = "FQDN";
const char kPasspointCredentialsDomainsProperty[] = "Domains";
const char kPasspointCredentialsRealmProperty[] = "Realm";
const char kPasspointCredentialsHomeOIsProperty[] = "HomeOIs";
const char kPasspointCredentialsRequiredHomeOIsProperty[] = "RequiredHomeOIs";
const char kPasspointCredentialsRoamingConsortiaProperty[] = "RoamingConsortia";
const char kPasspointCredentialsMeteredOverrideProperty[] = "MeteredOverride";
const char kPasspointCredentialsAndroidPackageNameProperty[] =
    "AndroidPackageName";
const char kPasspointCredentialsFriendlyNameProperty[] = "FriendlyName";
const char kPasspointCredentialsExpirationTimeMillisecondsProperty[] =
    "ExpirationTimeMilliseconds";

// Base Device property names.
const char kAddressProperty[] = "Address";  // Also used for IPConfig.
const char kInhibitedProperty[] = "Inhibited";
const char kIPConfigsProperty[] = "IPConfigs";
const char kIPv6DisabledProperty[] = "IPv6Disabled";
const char kInterfaceProperty[] = "Interface";  // Network interface name.
// kNameProperty: Defined above for Service. DEPRECATED (crbug.com/1011136).
const char kPoweredProperty[] = "Powered";
const char kScanIntervalProperty[] =
    "ScanInterval";                           // For both Cellular and WiFi.
const char kScanningProperty[] = "Scanning";  // For both Cellular and WiFi.
const char kSelectedServiceProperty[] = "SelectedService";

// Property names common to Cellular Device and Cellular Service.
const char kEidProperty[] = "Cellular.EID";
const char kIccidProperty[] = "Cellular.ICCID";
const char kImsiProperty[] = "Cellular.IMSI";

// kIccidProperty value when SIM card exists but ICCID is not available.
const char kUnknownIccid[] = "unknown-iccid";

// Cellular Device property names.
const char kCellularApnListProperty[] = "Cellular.APNList";
const char kCellularPolicyAllowRoamingProperty[] =
    "Cellular.PolicyAllowRoaming";
const char kDeviceIdProperty[] = "Cellular.DeviceID";
const char kEquipmentIdProperty[] = "Cellular.EquipmentID";
const char kEsnProperty[] = "Cellular.ESN";
const char kFirmwareRevisionProperty[] = "Cellular.FirmwareRevision";
const char kFoundNetworksProperty[] = "Cellular.FoundNetworks";
const char kHardwareRevisionProperty[] = "Cellular.HardwareRevision";
const char kHomeProviderProperty[] = "Cellular.HomeProvider";
const char kImeiProperty[] = "Cellular.IMEI";
const char kManufacturerProperty[] = "Cellular.Manufacturer";
const char kMdnProperty[] = "Cellular.MDN";
const char kMeidProperty[] = "Cellular.MEID";
const char kModelIdProperty[] = "Cellular.ModelID";
const char kMinProperty[] = "Cellular.MIN";
const char kProviderRequiresRoamingProperty[] =
    "Cellular.ProviderRequiresRoaming";
const char kSelectedNetworkProperty[] = "Cellular.SelectedNetwork";
const char kSIMPresentProperty[] = "Cellular.SIMPresent";
const char kSIMSlotInfoProperty[] = "Cellular.SIMSlotInfo";
const char kSupportNetworkScanProperty[] = "Cellular.SupportNetworkScan";
const char kUseAttachAPNProperty[] = "Cellular.UseAttachAPN";

const char kDBusObjectProperty[] = "DBus.Object";
const char kDBusServiceProperty[] = "DBus.Service";

// Ethernet Device property names.
const char kEapAuthenticationCompletedProperty[] = "EapAuthenticationCompleted";
const char kEapAuthenticatorDetectedProperty[] = "EapAuthenticatorDetected";
const char kDeviceBusTypeProperty[] = "Ethernet.DeviceBusType";
const char kLinkUpProperty[] = "Ethernet.LinkUp";
const char kUsbEthernetMacAddressSourceProperty[] =
    "Ethernet.UsbEthernetMacAddressSource";

// WiFi Device property names.
const char kBgscanMethodProperty[] = "BgscanMethod";
const char kBgscanShortIntervalProperty[] = "BgscanShortInterval";
const char kBgscanSignalThresholdProperty[] = "BgscanSignalThreshold";
const char kForceWakeToScanTimerProperty[] = "ForceWakeToScanTimer";
const char kLastWakeReasonProperty[] = "LastWakeReason";
const char kLinkStatisticsProperty[] = "LinkStatistics";
const char kMacAddressRandomizationEnabledProperty[] =
    "MACAddressRandomizationEnabled";
const char kMacAddressRandomizationSupportedProperty[] =
    "MACAddressRandomizationSupported";
const char kNetDetectScanPeriodSecondsProperty[] = "NetDetectScanPeriodSeconds";
const char kPasspointInterworkingSelectEnabledProperty[] =
    "PasspointInterworkingSelectEnabled";
const char kWakeOnWiFiSupportedProperty[] = "WakeOnWiFiSupported";
const char kWakeOnWiFiAllowedProperty[] = "WakeOnWiFiAllowed";
const char kWakeOnWiFiFeaturesEnabledProperty[] = "WakeOnWiFiFeaturesEnabled";
const char kWakeToScanPeriodSecondsProperty[] = "WakeToScanPeriodSeconds";

// Profile property names.
const char kEntriesProperty[] = "Entries";
// kNameProperty: Defined above for Service.
// kServicesProperty: Defined above for Manager.
const char kUserHashProperty[] = "UserHash";
const char kAlwaysOnVpnModeProperty[] = "AlwaysOnVpnMode";
const char kAlwaysOnVpnServiceProperty[] = "AlwaysOnVpnService";

// WiFi Service VendorInformation dictionary keys.
const char kVendorOUIListProperty[] = "OUIList";
const char kVendorWPSDeviceNameProperty[] = "DeviceName";
const char kVendorWPSManufacturerProperty[] = "Manufacturer";
const char kVendorWPSModelNameProperty[] = "ModelName";
const char kVendorWPSModelNumberProperty[] = "ModelNumber";

// Flimflam state options.
const char kStateIdle[] = "idle";
const char kStateAssociation[] = "association";
const char kStateConfiguration[] = "configuration";
const char kStateReady[] = "ready";
const char kStateNoConnectivity[] = "no-connectivity";
const char kStateRedirectFound[] = "redirect-found";
const char kStatePortalSuspected[] = "portal-suspected";
// TODO(b/260792466): This state is only used in the State property of Manager.
// Consider changing the to kStateIdle too, or removing the State property
// altogether if nothing depends on it.
const char kStateOffline[] = "offline";
const char kStateOnline[] = "online";
const char kStateDisconnect[] = "disconnecting";
const char kStateFailure[] = "failure";

// Shill WiFi roam state options.
const char kRoamStateIdle[] = "idle";
const char kRoamStateAssociation[] = "association";
const char kRoamStateConfiguration[] = "configuration";
const char kRoamStateReady[] = "ready";

// Shill Passpoint match type options.
const char kPasspointMatchTypeHome[] = "home";
const char kPasspointMatchTypeRoaming[] = "roaming";
const char kPasspointMatchTypeUnknown[] = "unknown";

// Flimflam portal phase and status.
const char kPortalDetectionPhaseConnection[] = "Connection";
const char kPortalDetectionPhaseDns[] = "DNS";
const char kPortalDetectionPhaseHttp[] = "HTTP";
const char kPortalDetectionPhaseContent[] = "Content";
const char kPortalDetectionPhaseUnknown[] = "Unknown";
const char kPortalDetectionStatusFailure[] = "Failure";
const char kPortalDetectionStatusTimeout[] = "Timeout";
const char kPortalDetectionStatusSuccess[] = "Success";
const char kPortalDetectionStatusRedirect[] = "Redirect";

// Flimflam property names for SIMLock status.
// kSIMLockStatusProperty is a Cellular Device property.
const char kSIMLockStatusProperty[] = "Cellular.SIMLockStatus";
const char kSIMLockTypeProperty[] = "LockType";
const char kSIMLockRetriesLeftProperty[] = "RetriesLeft";
const char kSIMLockEnabledProperty[] = "LockEnabled";

// Shill SIMSlotInfo properties.
const char kSIMSlotInfoEID[] = "EID";
const char kSIMSlotInfoICCID[] = "ICCID";
const char kSIMSlotInfoPrimary[] = "Primary";

// Flimflam property names for Cellular.FoundNetworks.
const char kLongNameProperty[] = "long_name";
const char kStatusProperty[] = "status";
const char kShortNameProperty[] = "short_name";
const char kTechnologyProperty[] = "technology";
const char kNetworkIdProperty[] = "network_id";

// Flimflam SIMLock status types.
const char kSIMLockPin[] = "sim-pin";
const char kSIMLockPuk[] = "sim-puk";

// APN info property names.
const char kApnProperty[] = "apn";
const char kApnNetworkIdProperty[] = "network_id";
const char kApnUsernameProperty[] = "username";
const char kApnPasswordProperty[] = "password";
const char kApnNameProperty[] = "name";
const char kApnLocalizedNameProperty[] = "localized_name";
const char kApnLanguageProperty[] = "language";
const char kApnAuthenticationProperty[] = "authentication";
// TODO(b/251551314): Remove kApnAttachProperty after 2025Q2
const char kApnAttachProperty[] = "attach";
const char kApnIpTypeProperty[] = "ip_type";
const char kApnTypesProperty[] = "apn_types";
const char kApnIdProperty[] = "id";
const char kApnSourceProperty[] = "apn_source";

// APN authentication property values (as expected by ModemManager).
const char kApnAuthenticationPap[] = "pap";
const char kApnAuthenticationChap[] = "chap";

// IP type property values.
const char kApnIpTypeV4[] = "ipv4";
const char kApnIpTypeV6[] = "ipv6";
const char kApnIpTypeV4V6[] = "ipv4v6";

// APN type property values.
const char kApnTypeDefault[] = "DEFAULT";
const char kApnTypeIA[] = "IA";
const char kApnTypeDun[] = "DUN";

// APN source property values.
const char kApnSourceAdmin[] = "admin";
const char kApnSourceUi[] = "ui";

// Payment Portal property names.
const char kPaymentPortalURL[] = "url";
const char kPaymentPortalMethod[] = "method";
const char kPaymentPortalPostData[] = "postdata";

// Operator info property names.
const char kOperatorNameKey[] = "name";
const char kOperatorCodeKey[] = "code";
const char kOperatorCountryKey[] = "country";
const char kOperatorUuidKey[] = "uuid";

// Flimflam network technology options.
const char kNetworkTechnology1Xrtt[] = "1xRTT";
const char kNetworkTechnologyEvdo[] = "EVDO";
const char kNetworkTechnologyGsm[] = "GSM";
const char kNetworkTechnologyGprs[] = "GPRS";
const char kNetworkTechnologyEdge[] = "EDGE";
const char kNetworkTechnologyUmts[] = "UMTS";
const char kNetworkTechnologyHspa[] = "HSPA";
const char kNetworkTechnologyHspaPlus[] = "HSPA+";
const char kNetworkTechnologyLte[] = "LTE";
const char kNetworkTechnologyLteAdvanced[] = "LTE Advanced";
const char kNetworkTechnology5gNr[] = "5GNR";

// Flimflam roaming state options
const char kRoamingStateHome[] = "home";
const char kRoamingStateRoaming[] = "roaming";
const char kRoamingStateUnknown[] = "unknown";

// Flimflam activation state options
const char kActivationStateActivated[] = "activated";
const char kActivationStateActivating[] = "activating";
const char kActivationStateNotActivated[] = "not-activated";
const char kActivationStatePartiallyActivated[] = "partially-activated";
const char kActivationStateUnknown[] = "unknown";

// Flimflam EAP method options.
const char kEapMethodPEAP[] = "PEAP";
const char kEapMethodTLS[] = "TLS";
const char kEapMethodTTLS[] = "TTLS";
const char kEapMethodLEAP[] = "LEAP";
const char kEapMethodMSCHAPV2[] = "MSCHAPV2";

// Flimflam EAP phase 2 auth options.
const char kEapPhase2AuthPEAPMD5[] = "auth=MD5";
const char kEapPhase2AuthPEAPMSCHAPV2[] = "auth=MSCHAPV2";
const char kEapPhase2AuthPEAPGTC[] = "auth=GTC";
const char kEapPhase2AuthTTLSMD5[] = "autheap=MD5";  // crosbug/26822
const char kEapPhase2AuthTTLSEAPMD5[] = "autheap=MD5";
const char kEapPhase2AuthTTLSEAPMSCHAPV2[] = "autheap=MSCHAPV2";
const char kEapPhase2AuthTTLSMSCHAPV2[] = "auth=MSCHAPV2";
const char kEapPhase2AuthTTLSMSCHAP[] = "auth=MSCHAP";
const char kEapPhase2AuthTTLSPAP[] = "auth=PAP";
const char kEapPhase2AuthTTLSCHAP[] = "auth=CHAP";
const char kEapPhase2AuthTTLSGTC[] = "auth=GTC";
const char kEapPhase2AuthTTLSEAPGTC[] = "autheap=GTC";

// Flimflam EAP TLS versions.
const char kEapTLSVersion1p0[] = "1.0";
const char kEapTLSVersion1p1[] = "1.1";
const char kEapTLSVersion1p2[] = "1.2";

// Flimflam VPN provider types.
const char kProviderArcVpn[] = "arcvpn";
const char kProviderIKEv2[] = "ikev2";
const char kProviderL2tpIpsec[] = "l2tpipsec";
const char kProviderOpenVpn[] = "openvpn";
const char kProviderThirdPartyVpn[] = "thirdpartyvpn";
const char kProviderWireGuard[] = "wireguard";

// Flimflam monitored properties
const char kMonitorPropertyChanged[] = "PropertyChanged";

// Flimflam type options.
const char kTypeEthernet[] = "ethernet";
const char kTypeWifi[] = "wifi";
const char kTypeCellular[] = "cellular";
const char kTypeVPN[] = "vpn";

// Flimflam mode options.
const char kModeManaged[] = "managed";

// WiFi SecurityClass options.
const char kSecurityClassNone[] = "none";
const char kSecurityClassWep[] = "wep";
const char kSecurityClassPsk[] = "psk";
const char kSecurityClass8021x[] = "802_1x";
// These two are deprecated.  Use kSecurityClass* equivalents above.
// TODO(b/226138492) Remove this once references in Chrome and Shill are
// removed.
const char kSecurityPsk[] = "psk";
const char kSecurity8021x[] = "802_1x";

// WiFi Security options.
const char kSecurityNone[] = "none";
const char kSecurityWep[] = "wep";
const char kSecurityWpa[] = "wpa";
const char kSecurityWpaWpa2[] = "wpa+wpa2";
const char kSecurityWpaAll[] = "wpa-all";
// Deprecated.  Use kSecurityWpa2 instead.
// TODO(b/226138492) Remove this once references in Chrome and Shill are
// removed.
const char kSecurityRsn[] = "rsn";
const char kSecurityWpa2[] = "wpa2";
const char kSecurityWpa2Wpa3[] = "wpa2+wpa3";
const char kSecurityWpa3[] = "wpa3";
const char kSecurityWpaEnterprise[] = "wpa-ent";
const char kSecurityWpaWpa2Enterprise[] = "wpa+wpa2-ent";
const char kSecurityWpaAllEnterprise[] = "wpa-all-ent";
const char kSecurityWpa2Enterprise[] = "wpa2-ent";
const char kSecurityWpa2Wpa3Enterprise[] = "wpa2+wpa3-ent";
const char kSecurityWpa3Enterprise[] = "wpa3-ent";

// WiFi Band options.
const char kBand2GHz[] = "2.4GHz";
const char kBand5GHz[] = "5GHz";
const char kBand6GHz[] = "6GHz";
const char kBandAll[] = "all-bands";
const char kBandUnknown[] = "unknown";

// Compress option values as expected by OpenVPN.
const char kOpenVPNCompressFramingOnly[] = "";
const char kOpenVPNCompressLz4[] = "lz4";
const char kOpenVPNCompressLz4V2[] = "lz4-v2";
const char kOpenVPNCompressLzo[] = "lzo";

// FlimFlam technology family options
const char kTechnologyFamilyCdma[] = "CDMA";
const char kTechnologyFamilyGsm[] = "GSM";

// IPConfig type options.
const char kTypeIPv4[] = "ipv4";
const char kTypeIPv6[] = "ipv6";
const char kTypeDHCP[] = "dhcp";
const char kTypeBOOTP[] = "bootp";
const char kTypeZeroConf[] = "zeroconf";
const char kTypeDHCP6[] = "dhcp6";
// kTypeVPN[] = "vpn" is defined above in device type session.

// Flimflam error options.
const char kErrorAaaFailed[] = "aaa-failed";
const char kErrorActivationFailed[] = "activation-failed";
const char kErrorBadPassphrase[] = "bad-passphrase";
const char kErrorBadWEPKey[] = "bad-wepkey";
const char kErrorConnectFailed[] = "connect-failed";
const char kErrorDNSLookupFailed[] = "dns-lookup-failed";
const char kErrorDhcpFailed[] = "dhcp-failed";
const char kErrorHTTPGetFailed[] = "http-get-failed";
const char kErrorInternal[] = "internal-error";
const char kErrorInvalidFailure[] = "invalid-failure";
const char kErrorIpsecCertAuthFailed[] = "ipsec-cert-auth-failed";
const char kErrorIpsecPskAuthFailed[] = "ipsec-psk-auth-failed";
const char kErrorNeedEvdo[] = "need-evdo";
const char kErrorNeedHomeNetwork[] = "need-home-network";
const char kErrorNoFailure[] = "no-failure";
const char kErrorNotAssociated[] = "not-associated";
const char kErrorNotAuthenticated[] = "not-authenticated";
const char kErrorOtaspFailed[] = "otasp-failed";
const char kErrorOutOfRange[] = "out-of-range";
const char kErrorPinMissing[] = "pin-missing";
const char kErrorPppAuthFailed[] = "ppp-auth-failed";
const char kErrorSimLocked[] = "sim-locked";
const char kErrorNotRegistered[] = "not-registered";
const char kErrorTooManySTAs[] = "too-many-stas";
const char kErrorDisconnect[] = "disconnect-failure";
const char kErrorUnknownFailure[] = "unknown-failure";

// Flimflam error result codes.
const char kErrorResultSuccess[] = "org.chromium.flimflam.Error.Success";
const char kErrorResultFailure[] = "org.chromium.flimflam.Error.Failure";
const char kErrorResultAlreadyConnected[] =
    "org.chromium.flimflam.Error.AlreadyConnected";
const char kErrorResultAlreadyExists[] =
    "org.chromium.flimflam.Error.AlreadyExists";
const char kErrorResultIllegalOperation[] =
    "org.chromium.flimflam.Error.IllegalOperation";
const char kErrorResultIncorrectPin[] =
    "org.chromium.flimflam.Error.IncorrectPin";
const char kErrorResultInProgress[] = "org.chromium.flimflam.Error.InProgress";
const char kErrorResultInternalError[] =
    "org.chromium.flimflam.Error.InternalError";
const char kErrorResultInvalidApn[] = "org.chromium.flimflam.Error.InvalidApn";
const char kErrorResultInvalidArguments[] =
    "org.chromium.flimflam.Error.InvalidArguments";
const char kErrorResultInvalidNetworkName[] =
    "org.chromium.flimflam.Error.InvalidNetworkName";
const char kErrorResultInvalidPassphrase[] =
    "org.chromium.flimflam.Error.InvalidPassphrase";
const char kErrorResultInvalidProperty[] =
    "org.chromium.flimflam.Error.InvalidProperty";
const char kErrorResultNoCarrier[] = "org.chromium.flimflam.Error.NoCarrier";
const char kErrorResultNotConnected[] =
    "org.chromium.flimflam.Error.NotConnected";
const char kErrorResultNotFound[] = "org.chromium.flimflam.Error.NotFound";
const char kErrorResultNotImplemented[] =
    "org.chromium.flimflam.Error.NotImplemented";
const char kErrorResultNotOnHomeNetwork[] =
    "org.chromium.flimflam.Error.NotOnHomeNetwork";
const char kErrorResultNotRegistered[] =
    "org.chromium.flimflam.Error.NotRegistered";
const char kErrorResultNotSupported[] =
    "org.chromium.flimflam.Error.NotSupported";
const char kErrorResultOperationAborted[] =
    "org.chromium.flimflam.Error.OperationAborted";
const char kErrorResultOperationInitiated[] =
    "org.chromium.flimflam.Error.OperationInitiated";
const char kErrorResultOperationTimeout[] =
    "org.chromium.flimflam.Error.OperationTimeout";
const char kErrorResultPassphraseRequired[] =
    "org.chromium.flimflam.Error.PassphraseRequired";
const char kErrorResultPermissionDenied[] =
    "org.chromium.flimflam.Error.PermissionDenied";
const char kErrorResultPinBlocked[] = "org.chromium.flimflam.Error.PinBlocked";
const char kErrorResultPinRequired[] =
    "org.chromium.flimflam.Error.PinRequired";
const char kErrorResultTechnologyNotAvailable[] =
    "org.chromium.flimflam.Error.TechnologyNotAvailable";
const char kErrorResultWepNotSupported[] =
    "org.chromium.flimflam.Error.WepNotSupported";
const char kErrorResultWrongState[] = "org.chromium.flimflam.Error.WrongState";

const char kUnknownString[] = "UNKNOWN";

// Device bus types.
const char kDeviceBusTypePci[] = "pci";
const char kDeviceBusTypeUsb[] = "usb";

// Technology types (augments "Flimflam type options" above).
const char kTypeEthernetEap[] = "etherneteap";
const char kTypeTunnel[] = "tunnel";
const char kTypeLoopback[] = "loopback";
const char kTypePPP[] = "ppp";
const char kTypeGuestInterface[] = "guest_interface";
const char kTypeUnknown[] = "unknown";

// Error strings.
const char kErrorEapAuthenticationFailed[] = "eap-authentication-failed";
const char kErrorEapLocalTlsFailed[] = "eap-local-tls-failed";
const char kErrorEapRemoteTlsFailed[] = "eap-remote-tls-failed";

// Subject alternative name match type property values as expected by
// wpa_supplicant.
const char kEapSubjectAlternativeNameMatchTypeEmail[] = "EMAIL";
const char kEapSubjectAlternativeNameMatchTypeDNS[] = "DNS";
const char kEapSubjectAlternativeNameMatchTypeURI[] = "URI";

// WiFi Device kLinkStatisticsProperty sub-property names.
const char kAverageReceiveSignalDbmProperty[] = "AverageReceiveSignalDbm";
const char kByteReceiveSuccessesProperty[] = "ByteReceiveSuccesses";
const char kByteTransmitSuccessesProperty[] = "ByteTransmitSuccesses";
const char kInactiveTimeMillisecondsProperty[] = "InactiveTimeMilliseconds";
const char kLastReceiveSignalDbmProperty[] = "LastReceiveSignalDbm";
const char kPacketReceiveDropProperty[] = "PacketReceiveDrops";
const char kPacketReceiveSuccessesProperty[] = "PacketReceiveSuccesses";
const char kPacketTransmitFailuresProperty[] = "PacketTransmitFailures";
const char kPacketTransmitSuccessesProperty[] = "PacketTransmitSuccesses";
const char kReceiveBitrateProperty[] = "ReceiveBitrate";
const char kTransmitBitrateProperty[] = "TransmitBitrate";
const char kTransmitRetriesProperty[] = "TransmitRetries";

// Wake on WiFi features.
const char kWakeOnWiFiFeaturesEnabledDarkConnect[] = "darkconnect";
const char kWakeOnWiFiFeaturesEnabledNone[] = "none";

// Wake on WiFi wake reasons.
// These (except Unknown) will also be sent to powerd via
// RecordDarkResumeWakeReason, to tell it the reason of the current dark
// resume.
const char kWakeOnWiFiReasonDisconnect[] = "WiFi.Disconnect";
const char kWakeOnWiFiReasonPattern[] = "WiFi.Pattern";
const char kWakeOnWiFiReasonSSID[] = "WiFi.SSID";
const char kWakeOnWiFiReasonUnknown[] = "Unknown";

// kEapKeyMgmtProperty values.
const char kKeyManagementIEEE8021X[] = "IEEE8021X";

// Wake on WiFi Packet Type Constants.
const char kWakeOnTCP[] = "TCP";
const char kWakeOnUDP[] = "UDP";
const char kWakeOnIDP[] = "IDP";
const char kWakeOnIPIP[] = "IPIP";
const char kWakeOnIGMP[] = "IGMP";
const char kWakeOnICMP[] = "ICMP";
const char kWakeOnIP[] = "IP";

// ONC Source constants.
static constexpr char kONCSourceUnknown[] = "Unknown";
static constexpr char kONCSourceNone[] = "None";
static constexpr char kONCSourceUserImport[] = "UserImport";
static constexpr char kONCSourceDevicePolicy[] = "DevicePolicy";
static constexpr char kONCSourceUserPolicy[] = "UserPolicy";

// MAC Randomization constants
static constexpr char kWifiRandomMacPolicyHardware[] = "Hardware";
static constexpr char kWifiRandomMacPolicyFullRandom[] = "FullRandom";
static constexpr char kWifiRandomMacPolicyOUIRandom[] = "OUIRandom";
static constexpr char kWifiRandomMacPolicyPersistentRandom[] =
    "PersistentRandom";
static constexpr char kWifiRandomMacPolicyNonPersistentRandom[] =
    "NonPersistentRandom";

// Cellular activation types.
const char kActivationTypeNonCellular[] = "NonCellular";  // For future use
const char kActivationTypeOMADM[] = "OMADM";              // For future use
const char kActivationTypeOTA[] = "OTA";
const char kActivationTypeOTASP[] = "OTASP";

// USB Ethernet MAC address sources.
const char kUsbEthernetMacAddressSourceDesignatedDockMac[] =
    "designated_dock_mac";
const char kUsbEthernetMacAddressSourceBuiltinAdapterMac[] =
    "builtin_adapter_mac";
const char kUsbEthernetMacAddressSourceUsbAdapterMac[] = "usb_adapter_mac";

// Geolocation property field names.
// Reference:
//    https://devsite.googleplex.com/maps/documentation/business/geolocation/
// Top level properties for a Geolocation request.
const char kGeoHomeMobileCountryCodeProperty[] = "homeMobileCountryCode";
const char kGeoHomeMobileNetworkCodeProperty[] = "homeMobileNetworkCode";
const char kGeoRadioTypePropertyProperty[] = "radioType";
const char kGeoCellTowersProperty[] = "cellTowers";
const char kGeoWifiAccessPointsProperty[] = "wifiAccessPoints";
// Cell tower object property names.
const char kGeoCellIdProperty[] = "cellId";
const char kGeoLocationAreaCodeProperty[] = "locationAreaCode";
const char kGeoMobileCountryCodeProperty[] = "mobileCountryCode";
const char kGeoMobileNetworkCodeProperty[] = "mobileNetworkCode";
const char kGeoTimingAdvanceProperty[] = "timingAdvance";
// WiFi access point property names.
const char kGeoMacAddressProperty[] = "macAddress";
const char kGeoChannelProperty[] = "channel";
const char kGeoSignalToNoiseRatioProperty[] = "signalToNoiseRatio";
// Common property names for geolocation objects.
const char kGeoAgeProperty[] = "age";
const char kGeoSignalStrengthProperty[] = "signalStrength";
// ThirdPartyVpn parameters and constants.
const char kAddressParameterThirdPartyVpn[] = "address";
const char kBroadcastAddressParameterThirdPartyVpn[] = "broadcast_address";
const char kGatewayParameterThirdPartyVpn[] = "gateway";
const char kBypassTunnelForIpParameterThirdPartyVpn[] = "bypass_tunnel_for_ip";
const char kSubnetPrefixParameterThirdPartyVpn[] = "subnet_prefix";
const char kMtuParameterThirdPartyVpn[] = "mtu";
const char kDomainSearchParameterThirdPartyVpn[] = "domain_search";
const char kDnsServersParameterThirdPartyVpn[] = "dns_servers";
const char kInclusionListParameterThirdPartyVpn[] = "inclusion_list";
const char kExclusionListParameterThirdPartyVpn[] = "exclusion_list";
const char kReconnectParameterThirdPartyVpn[] = "reconnect";
const char kObjectPathBase[] = "/thirdpartyvpn/";
const char kNonIPDelimiter = ':';
const char kIPDelimiter = ' ';

// Always-on VPN modes for the kAlwaysOnVpnModeProperty Profile property.
const char kAlwaysOnVpnModeOff[] = "off";
const char kAlwaysOnVpnModeBestEffort[] = "best-effort";
const char kAlwaysOnVpnModeStrict[] = "strict";

// Possible traffic sources. Note that these sources should be kept in sync with
// the sources defined in TrafficCounter::Source at:
// src/platform2/system_api/dbus/patchpanel/patchpanel_service.proto
const char kTrafficCounterSourceUnknown[] = "unknown";
const char kTrafficCounterSourceChrome[] = "chrome";
const char kTrafficCounterSourceUser[] = "user";
const char kTrafficCounterSourceArc[] = "arc";
const char kTrafficCounterSourceCrosvm[] = "crosvm";
const char kTrafficCounterSourcePluginvm[] = "pluginvm";
const char kTrafficCounterSourceUpdateEngine[] = "update_engine";
const char kTrafficCounterSourceVpn[] = "vpn";
const char kTrafficCounterSourceSystem[] = "system";

// Manager kTetheringConfigProperty dictionary key names.
const char kTetheringConfAutoDisableProperty[] = "auto_disable";
const char kTetheringConfBandProperty[] = "band";
const char kTetheringConfMARProperty[] = "randomize_mac_address";
const char kTetheringConfPassphraseProperty[] = "passphrase";
const char kTetheringConfSecurityProperty[] = "security";
const char kTetheringConfSSIDProperty[] = "ssid";
const char kTetheringConfUpstreamTechProperty[] = "upstream_technology";

// Manager kTetheringCapabilitiesProperty dictionary key names.
const char kTetheringCapDownstreamProperty[] = "downstream_technologies";
const char kTetheringCapSecurityProperty[] = "wifi_security_modes";
const char kTetheringCapUpstreamProperty[] = "upstream_technologies";

// Manager kTetheringStatusProperty dictionary key names.
const char kTetheringStatusClientHostnameProperty[] = "hostname";
const char kTetheringStatusClientIPv4Property[] = "IPv4";
const char kTetheringStatusClientIPv6Property[] = "IPv6";
const char kTetheringStatusClientMACProperty[] = "MAC";
const char kTetheringStatusClientsProperty[] = "active_clients";
const char kTetheringStatusDownstreamTechProperty[] = "downstream_technology";
const char kTetheringStatusIdleReasonProperty[] = "idle_reason";
const char kTetheringStatusStateProperty[] = "state";
const char kTetheringStatusUpstreamTechProperty[] = "upstream_technology";
const char kTetheringStatusUpstreamServiceProperty[] = "upstream_service";

// kTetheringStatusIdleReasonProperty values
const char kTetheringIdleReasonClientStop[] = "client_stop";
const char kTetheringIdleReasonError[] = "error";
const char kTetheringIdleReasonInactive[] = "inactive";
const char kTetheringIdleReasonInitialState[] = "initial_state";
const char kTetheringIdleReasonSuspend[] = "suspend";
const char kTetheringIdleReasonUpstreamDisconnect[] = "upstream_disconnect";
const char kTetheringIdleReasonUserExit[] = "user_exit";

// kTetheringStatusStateProperty values
const char kTetheringStateActive[] = "active";
const char kTetheringStateIdle[] = "idle";
const char kTetheringStateStarting[] = "starting";

// SetTetheringEnabled result values
const char kTetheringEnableResultFailure[] = "failure";
const char kTetheringEnableResultInvalidProperties[] = "invalid_properties";
const char kTetheringEnableResultNotAllowed[] = "not_allowed";
const char kTetheringEnableResultSuccess[] = "success";
const char kTetheringEnableResultUpstreamNotAvailable[] =
    "upstream_not_available";

// kCheckTetheringReadinessFunction return status
const char kTetheringReadinessNotAllowed[] = "not_allowed";
const char kTetheringReadinessReady[] = "ready";
const char kTetheringReadinessUpstreamNetworkNotAvailable[] =
    "upstream_network_not_available";

}  // namespace shill

#endif  // SYSTEM_API_DBUS_SHILL_DBUS_CONSTANTS_H_
