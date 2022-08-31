project(
    'phosphor-buttons', 'cpp',
    version: '1.0.0',
    meson_version: '>=0.58.0',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20',
    ]
)

conf_data = configuration_data()
conf_data.set_quoted('POWER_DBUS_OBJECT_NAME', '/xyz/openbmc_project/Chassis/Buttons/Power0')
conf_data.set_quoted('RESET_DBUS_OBJECT_NAME', '/xyz/openbmc_project/Chassis/Buttons/Reset0')
conf_data.set_quoted('ID_DBUS_OBJECT_NAME', '/xyz/openbmc_project/Chassis/Buttons/ID0')
conf_data.set_quoted('HS_DBUS_OBJECT_NAME', '/xyz/openbmc_project/Chassis/Buttons/HostSelector')
conf_data.set_quoted('GPIO_BASE_LABEL_NAME', '1e780000.gpio')
conf_data.set_quoted('CHASSIS_STATE_OBJECT_NAME', '/xyz/openbmc_project/state/chassis')
conf_data.set_quoted('CHASSISSYSTEM_STATE_OBJECT_NAME', '/xyz/openbmc_project/state/chassis_system')
conf_data.set_quoted('HOST_STATE_OBJECT_NAME', '/xyz/openbmc_project/state/host')
conf_data.set_quoted('ID_LED_GROUP', get_option('id-led-group'))

conf_data.set('LONG_PRESS_TIME_MS', get_option('long-press-time-ms'))
conf_data.set('LOOKUP_GPIO_BASE', get_option('lookup-gpio-base').enabled())

configure_file(output: 'config.h',
    configuration: conf_data
)

sdbusplus_dep = dependency('sdbusplus')
phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')
gpioplus_dep = dependency('gpioplus')
libgpiod_dep = dependency('libgpiodcxx')

cpp = meson.get_compiler('cpp')
if cpp.has_header_symbol(
        'nlohmann/json.hpp',
        'nlohmann::json::string_t',
        required:false)
    nlohmann_json_dep = declare_dependency()
else
    nlohmann_json_dep = dependency('nlohmann-json')
endif

deps = [
    sdbusplus_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    nlohmann_json_dep,
    gpioplus_dep,
    libgpiod_dep,
]

sources_buttons = [
    'src/gpio.cpp',
    'src/hostSelector_switch.cpp',
    'src/id_button.cpp',
    'src/main.cpp',
    'src/power_button.cpp',
    'src/reset_button.cpp',
]

sources_handler = [
    'src/button_handler_main.cpp',
    'src/button_handler.cpp',
]

executable(
    'buttons',
    sources_buttons,
    implicit_include_directories: true,
    include_directories: ['inc'],
    dependencies: deps,
    install: true,
    install_dir: get_option('bindir')
)

executable(
    'button-handler',
    sources_handler,
    implicit_include_directories: true,
    include_directories: ['inc'],
    dependencies: deps,
    install: true,
    install_dir: get_option('bindir')
)

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_variable(
        pkgconfig: 'systemdsystemunitdir',
        pkgconfig_define: ['prefix', get_option('prefix')])

configure_file(input: 'service_files/phosphor-button-handler.service',
                output: 'phosphor-button-handler.service',
                copy: true,
                install_dir: systemd_system_unit_dir)

configure_file(input: 'service_files/xyz.openbmc_project.Chassis.Buttons.service',
                output: 'xyz.openbmc_project.Chassis.Buttons.service',
                copy: true,
                install_dir: systemd_system_unit_dir)
