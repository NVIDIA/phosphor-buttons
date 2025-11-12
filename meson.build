project(
    'phosphor-buttons',
    'cpp',
    version: '1.0.0',
    meson_version: '>=1.1.1',
    default_options: ['warning_level=3', 'werror=true', 'cpp_std=c++23'],
)

host_instances = get_option('host-instances')
formatted_instances = host_instances.replace(' ', ', ')

conf_data = configuration_data()
conf_data.set('INSTANCES', formatted_instances)
conf_data.set_quoted('ID_LED_GROUP', get_option('id-led-group'))
conf_data.set_quoted('POWER_BUTTON_PROFILE', get_option('power-button-profile'))
conf_data.set('LONG_PRESS_TIME_MS', get_option('long-press-time-ms'))
conf_data.set('LOOKUP_GPIO_BASE', get_option('lookup-gpio-base').allowed())
conf_data.set10(
    'ENABLE_RESET_BUTTON_DO_WARM_REBOOT',
    get_option('reset-button-do-warm-reboot').allowed(),
)

configure_file(
    input: 'meson_config.hpp.in',
    output: 'config.hpp',
    configuration: conf_data,
)

gpioplus_dep = dependency('gpioplus')
nlohmann_json_dep = dependency('nlohmann_json', include_type: 'system')
phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')
sdbusplus_dep = dependency('sdbusplus')
sdeventplus_dep = dependency('sdeventplus')

deps = [
    sdbusplus_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    nlohmann_json_dep,
    gpioplus_dep,
    sdeventplus_dep,
]

sources_buttons = [
    'src/gpio.cpp',
    'src/cpld.cpp',
    'src/hostSelector_switch.cpp',
    'src/debugHostSelector_button.cpp',
    'src/serial_uart_mux.cpp',
    'src/id_button.cpp',
    'src/main.cpp',
    'src/power_button.cpp',
    'src/reset_button.cpp',
]

sources_handler = [
    'src/button_handler_main.cpp',
    'src/button_handler.cpp',
    'src/host_then_chassis_poweroff.cpp',
]

executable(
    'buttons',
    sources_buttons,
    implicit_include_directories: true,
    include_directories: ['inc'],
    dependencies: deps,
    install: true,
    install_dir: get_option('bindir'),
)

executable(
    'button-handler',
    sources_handler,
    implicit_include_directories: true,
    include_directories: ['inc'],
    dependencies: deps,
    install: true,
    install_dir: get_option('bindir'),
)

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_variable(
    'systemd_system_unit_dir',
    pkgconfig_define: ['prefix', get_option('prefix')],
)

fs = import('fs')
fs.copyfile(
    'service_files/phosphor-button-handler.service',
    install: true,
    install_dir: systemd_system_unit_dir,
)
fs.copyfile(
    'service_files/xyz.openbmc_project.Chassis.Buttons.service',
    install: true,
    install_dir: systemd_system_unit_dir,
)
