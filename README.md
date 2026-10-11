COSMO GATEWAY Industrial Communications Gateway for Cloud Platform
Integration
=================================================================

1. OVERVIEW

Cosmo Gateway is a firmware project for an industrial communications
gateway. Its purpose is to act as an integration point between field
equipment or controllers and a cloud platform, supporting communication
management, device configuration, and data exchange.

The repository contains C source code and configuration files for the
Texas Instruments MSP-EXP432E401Y platform, as well as modules related
to networking, Wi-Fi, MQTT, CAN, RPC communication, storage, and event
management.

IMPORTANT: the presence of a module in the repository does not guarantee
that all of its functions are enabled in the current build or
production-ready. Actual behavior depends on the implementation, project
configuration, and connected hardware.

2. PLATFORM AND TECHNOLOGIES

-   Target platform identified in the project: MSP-EXP432E401Y.
-   Main programming language: C.
-   Runtime environment: the project includes main_tirtos.c and
    configuration associated with TI-RTOS.
-   Hardware configuration: SysConfig (.syscfg) files.
-   Communication and service components present in the source tree:
    Wi-Fi, MQTT client, CAN, RPC over UART, and components whose names
    are associated with Zigbee/Z-Stack.
-   Configuration and storage: modules for flash memory, SD card, file
    operations, and a configuration database.
-   Data processing: modules related to JSON and a service identified as
    smabit_cloud.

Before changing the build environment, verify the SDK, compiler, and
tool versions used by the original project.

3. REPOSITORY STRUCTURE

The following files and directories appear in the repository. Their
descriptions summarize their likely roles based on their names; inspect
the source code to confirm implementation details.

-   src/ Additional project source code.

-   ftp/ Directory related to FTP resources or components.

-   targetConfigs/ Debugging and target-device configuration files.

-   Cosmo_GW.cfg Project configuration file.

-   Cosmo_GW_6.syscfg and Cosmo_GW_6B.syscfg SysConfig files for
    configuring board peripherals and resources.

-   MSP_EXP432E401Y_TIRTOS.cmd Linker configuration file associated with
    the target and TI-RTOS.

-   main_tirtos.c / main_tirtos.h Firmware entry point and main
    initialization components.

-   network.c, network_bsd.c, network_if.c, and related .h files
    Components for the network layer and communication interfaces.

-   wifi.c / wifi.h Module associated with Wi-Fi connectivity.

-   mqttc.c / mqttc.h MQTT client module for exchanging messages with an
    MQTT broker.

-   can.c / can.h Module associated with CAN communication.

-   rpc.c, rpc.h, and rpcTransportUart.c Components for remote procedure
    calls and UART transport.

-   config-db.c / config-db.h Configuration data management.

-   config-fileops.c / config-fileops.h File-based configuration
    operations.

-   flash.c, sd.c / sd.h, and spiffsexternal.c Components related to
    flash memory, SD cards, and file-system storage.

-   jsonc.c / jsonc.h Functions related to JSON data handling.

-   event.c / event.h and evlog.c / evlog.h Event management and event
    logging.

-   driver-8io.c / driver-8io.h A module whose name suggests support for
    an eight-input/output device. Verify the exact signal mapping and
    supported hardware in the source code.

-   localapp.c / localapp.h Functions associated with the local
    application.

-   smabit_cloud.c A module identified by its name as related to the
    project’s cloud service or cloud integration.

-   pem_processor.c / pem_processor.h A processing module identified by
    the PEM name; its specific role must be confirmed in the source
    code.

-   buttonled.c / buttonled.h Button and LED functions.

-   timer_s.c / timer_s.h Timer-related functions.

-   memmgr.c Memory management.

-   mtAf., mtApp_config., mtParser., mtSapi., mtSys., mtZdo., and
    mtUtil.h Components whose names are associated with a communications
    stack or application interface. Confirm their functions and
    supported devices by reviewing the code and dependencies.

4. REFERENCE FUNCTIONAL ARCHITECTURE

At a conceptual level, the gateway can be organized into the following
layers:

1.  Field devices and controllers Devices connected through the
    interfaces available on the hardware. The repository includes a CAN
    module and serial communication components; actual compatibility
    depends on the implementation and connected equipment.

2.  Interfaces and drivers Code that initializes and manages hardware
    resources and communication interfaces configured for the board.

3.  Internal firmware services Configuration management, storage, event
    handling, timers, and message processing.

4.  Network connectivity Network and Wi-Fi components, depending on the
    active project configuration.

5.  External service integration The mqttc.c module indicates the
    presence of an MQTT client, while smabit_cloud.c suggests
    integration with a specific cloud service. Server addresses, topics,
    credentials, message formats, and supported operations should be
    documented after reviewing the relevant code and configuration.

This diagram in words describes a reference functional organization. It
does not guarantee that every layer is fully integrated or enabled in
the current version.

5. BUILD REQUIREMENTS

Before building, identify and use versions compatible with the original
project:

-   Code Composer Studio (CCS), or another environment explicitly
    configured for the project.
-   The Texas Instruments SDK required by the microcontroller and
    dependencies.
-   A C compiler compatible with the project configuration.
-   SysConfig tools for processing .syscfg files.
-   Drivers, libraries, and TI-RTOS components required by the
    configuration.
-   The MSP-EXP432E401Y board and any accessories needed to test its
    interfaces.

Recommended general workflow:

1.  Clone or download the repository.
2.  Open or import the project into a compatible development
    environment.
3.  Check SDK paths, dependencies, and build options.
4.  Review Cosmo_GW_6.syscfg and Cosmo_GW_6B.syscfg and generate the
    hardware configuration corresponding to the selected target.
5.  Build the project and resolve dependency or configuration errors.
6.  Flash the board and first validate startup and debug output.
7.  Test each communication interface separately before integration
    testing with industrial equipment or cloud services.

Menu names and import steps may differ depending on the SDK and Code
Composer Studio versions.

6. CONFIGURATION AND STARTUP

Before connecting the gateway to a network or real equipment, document
and verify:

-   Exact hardware variant and the SysConfig file to use.
-   Network settings and Wi-Fi credentials, if applicable.
-   MQTT broker address, port, authentication, certificates, and
    connection settings, if implemented.
-   Message formats, topics, and publish/subscribe rules.
-   Field interface, communication speed, and communication parameters.
-   Persistent configuration settings and behavior after a reboot.
-   Firmware update and recovery procedures.
-   Available indicators, logs, and diagnostic procedures.

Do not commit passwords, private keys, access tokens, or private
certificates to the public repository. Use appropriate configuration and
secure-storage mechanisms for the deployment environment.

7. RECOMMENDED TESTS

-   Startup: verify that the firmware starts without errors.
-   Configuration: verify parameter reading, writing, and persistence.
-   Networking: test connectivity and recovery after a connection loss.
-   MQTT: test connection, publishing, subscribing, and reconnection
    when these features are implemented and configured.
-   CAN and UART/RPC: test transmission, reception, errors, and timeouts
    using appropriate test equipment.
-   Storage: verify read/write operations and recovery after a reboot.
-   Events and logs: confirm that important errors can be diagnosed.
-   End-to-end integration: verify that field-device data reaches the
    intended platform and that remote commands, if supported, are
    processed safely.
-   Robustness: test network loss, reboots, invalid messages, and
    service recovery.

Start testing in a controlled environment. Do not connect the firmware
to critical industrial processes without a security assessment,
functional validation, and recovery procedures.

8. STATUS AND SCOPE

The repository contains modules that indicate a gateway firmware with
communication, configuration, and storage capabilities. The actual
available functionality must be confirmed by reviewing the source code,
building the firmware, and testing it on the target hardware.

Before treating a version as production-ready, document:

-   Implemented features and pending work.
-   Supported hardware models and peripherals.
-   Exact SDK, compiler, and library versions.
-   Protocols, data formats, and cloud services actually supported.
-   Configuration parameters and installation procedure.
-   Cybersecurity, recovery, and maintenance requirements.
-   Validation and test results.

9. LICENSE AND CONTRIBUTIONS

Review the repository’s license files and third-party notices before
redistributing the firmware or its dependencies. When contributing to
the project, describe the changes, tests performed, and any
modifications to the hardware or build configuration.

Repository: https://github.com/alejo-en-cuba-gh/Cosmo_Gateway

End of document.
