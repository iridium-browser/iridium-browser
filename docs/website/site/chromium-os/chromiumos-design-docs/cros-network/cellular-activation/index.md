---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
- - /chromium-os/chromiumos-design-docs/cros-network
  - 'CrOS network: notes on ChromiumOS networking'
page_name: cellular-activation
title: Cellular Activation (and Chrome/CrOS network management API)
---

ActivateCellular (network_menu.cc)

-&gt;
ash::Shell::GetInstance()-&gt;delegate()-&gt;OpenMobileSetup(cellular-&gt;service_path())

-&gt; MobileSetupDialog::Show(service_path) \[if kEnableMobileSetupDialog\]

-&gt; MobileSetupDialogDelegate::GetInstance()-&gt;ShowDialog(service_path)

-&gt; MobileSetupDialogDelegate::GetDialogContentURL \[presumably\]

// returns chrome::kChromeUIMobileSetupURL + service_path_

-&gt; browser-&gt;OpenURL(chrome::kChromeUIMobileSetupURL) \[otherwise; resolves
to chrome://mobilesetup\]

MobileActivator::SetTransactionStatus (mobile_activator.cc)

-&gt; StartOTASP

-&gt; EvaluateCellularNetwork

MobileActivator::OnNetworkManagerChanged (mobile_activator.cc)

-&gt; EvaluateCellularNetwork

MobileActivator::OnNetworkChanged (mobile_activator.cc)

-&gt; EvaluateCellularNetwork

MobileActivator::StartActivation

-&gt; EvaluateCelluarNetwork

Who calls OnNetworkManagerChanged?

- NetworkLibraryImplBase::NotifyNetworkManagerChanged

-&gt; NetworkLibraryImplBase::SignalNetworkManagerObservers

- NetworkMessageObserver::NetworkMessageObserver

- AshSystemTrayDelegate

- NetworkMenuButton

- NetworkScreen

MobileActivator::ChangeState (mobile_activator.cc)

-&gt; FOR_EACH_OBSERVER(Observer, observers_, OnActivationStateChanged(network,
state_, error_description));

-&gt; MobileSetupHandler::OnActivationStateChanged (mobile_seutp_ui.cc)
\[presumably\]

-&gt; web_ui()-&gt;CallJavascriptFunction(kJsDeviceStatusChangedCallback,
device_dict)

-&gt; mobile.MobileSetup.deviceStateChanged (mobile_setup.js)

-&gt; updateDeviceStatus_

-&gt; changeState_

-&gt; stopSpinner_ \[if PLAN_ACTIVATION_DONE\]

---

MobileSetupDialogDelegate::OnActivationStateChanged (mobile_setup_dialog.cc)

// does nothing

chromeos.connectionManager.setTransactionStatus (connection_manager.js)

-&gt; reportTransactionStatus_

-&gt; postMessage(msg, 'chrome://mobilesetup')

// seems to be billing related. see onMessageReceived_ in mobile_setup.js

NetworkLoginObserver

// The network login observer reshows a login dialog if there was an error.

NetworkMessageObserver

// The network message observer displays a system notification for network

// messages.

NetworkLibrary interface declarations

- NetworkManagerObserver::OnNetworkManagerChanged

- NetworkObserver::OnNetworkChanged

- NetworkDeviceObserver::OnNetworkDeviceChanged

- NetworkDeviceObserver::OnNetworkDeviceFoundNetworks

- NetworkDeviceObserver::OnNetworkDeviceSimLockChanged

- CellularDataPlanObserver::OnCellularDataPlanChanged

- PinOperationObserver::OnPinOperationCompleted

- UserActionObserver::OnConnectionInitiated

NetworkLibraryImplCros::NetworkManagerStatusChangedHandler

NetworkLibraryImplCros::NetworkManagerUpdate

-&gt; NetworkManagerStatusChanged

-&gt; NotifyNetworkManagerChanged \[ONLY for PROPERTY_INDEX_OFFLINE_MODE\]

CrosMonitorNetworkManagerProperties

-&gt; new NetworkManagerPropertiesWatcher

-&gt;
DBusThreadManager::Get()-&gt;GetFlimflamManagerClient()-&gt;SetPropertyChangedHandler

NetworkLibraryImplCros::UpdateNetworkStatus

-&gt; NotifyNetworkManagerChanged

NetworkLibraryImplCros::UpdateTechnologies

-&gt; NotifyNetworkManagerChanged

NetworkLibraryImplCros::ParseNetwork

-&gt; NotifyNetworkManagerChanged

NetworkLibraryImplCros::ParseNetworkDevice

-&gt; NotifyNetworkManagerChanged

AshSystemTrayDelegate: updates status icons

---

Network::SetState (network_library.cc)

// processes state change events from connection manager
