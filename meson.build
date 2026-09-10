project(
    'phosphor-buttons',
    'cpp',
    version: '1.0.0',
    meson_version: '>=1.1.1',
    default_options: ['warning_level=3', 'werror=true', 'cpp_std=c++23'],
)

conf_data = configuration_data()
conf_data.set_quoted(
    'POWER_DBUS_OBJECT_NAME',
    '/xyz/openbmc_project/Chassis/Buttons/Power0',
)
conf_data.set_quoted(
    'RESET_DBUS_OBJECT_NAME',
    '/xyz/openbmc_project/Chassis/Buttons/Reset0',
)
conf_data.set_quoted(
    'ID_DBUS_OBJECT_NAME',
    '/xyz/openbmc_project/Chassis/Buttons/ID0',
)
conf_data.set_quoted(
    'HS_DBUS_OBJECT_NAME',
    '/xyz/openbmc_project/Chassis/Buttons/HostSelector',
)
conf_data.set_quoted('GPIO_BASE_LABEL_NAME', '1e780000.gpio')
conf_data.set_quoted(
    'CHASSIS_STATE_OBJECT_NAME',
    '/xyz/openbmc_project/state/chassis',
)
conf_data.set_quoted(
    'CHASSISSYSTEM_STATE_OBJECT_NAME',
    '/xyz/openbmc_project/state/chassis_system',
)
conf_data.set_quoted('HOST_STATE_OBJECT_NAME', '/xyz/openbmc_project/state/host')
conf_data.set_quoted('ID_LED_GROUP', get_option('id-led-group'))

conf_data.set('LONG_PRESS_TIME_MS', get_option('long-press-time-ms'))
conf_data.set('LOOKUP_GPIO_BASE', get_option('lookup-gpio-base').enabled())

uid_button = get_option('uid-button-function')
conf_data.set10('UID_BUTTON_FUNCTION', uid_button.allowed())
uid_password_reset_time = get_option('uid-password-reset-time')
uid_factory_reset_time = get_option('uid-factory-reset-time')
uid_password_reset_hint_time = get_option('uid-password-reset-hint-ms')
uid_factory_reset_hint_time = get_option('uid-factory-reset-hint-ms')
if uid_button.allowed()
    if uid_password_reset_time <= 0 or uid_factory_reset_time <= 0
        error('UID reset thresholds must be > 0 ms')
    endif
    if uid_password_reset_time >= uid_factory_reset_time
        error('uid-password-reset-time must be less than uid-factory-reset-time')
    endif
    if get_option('username') == ''
        error('username must be set when uid-button-function is enabled')
    endif
    if get_option('password') == ''
        error('password must be set when uid-button-function is enabled')
    endif
    if get_option('uid-reset-hint') == ''
        error('uid-reset-hint must be set when uid-button-function is enabled')
    endif
    if uid_password_reset_hint_time <= 0 or uid_factory_reset_hint_time <= 0
        error('UID reset hint durations must be > 0 ms')
    endif
    # Write the credentials into the generated config.h header rather than
    # passing them via -D on the compiler command line. This keeps the
    # plaintext out of compile command lines, compile_commands.json, and
    # build logs.
    conf_data.set_quoted('USERNAME', get_option('username'))
    conf_data.set_quoted('PASSWORD', get_option('password'))
endif
conf_data.set('UID_PASSWORD_RESET_TIME_MSEC', uid_password_reset_time)
conf_data.set('UID_FACTORY_RESET_TIME_MSEC', uid_factory_reset_time)

# Blink rate comes from the group's Period in the platform led.yaml.
conf_data.set_quoted('UID_RESET_HINT_LED_GROUP', get_option('uid-reset-hint'))
conf_data.set('UID_PASSWORD_RESET_HINT_MSEC', uid_password_reset_hint_time)
conf_data.set('UID_FACTORY_RESET_HINT_MSEC', uid_factory_reset_hint_time)

configure_file(output: 'config.h', configuration: conf_data)

sdbusplus_dep = dependency('sdbusplus')
phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')
gpioplus_dep = dependency('gpioplus')
libgpiod_dep = dependency('libgpiodcxx')

cpp = meson.get_compiler('cpp')
if cpp.has_header_symbol(
    'nlohmann/json.hpp',
    'nlohmann::json::string_t',
    required: false,
)
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

sources_handler = ['src/button_handler_main.cpp', 'src/button_handler.cpp']

# The UID button password/factory reset runs inside button-handler, so only
# that executable needs PAM. The credentials come from config.h (set above),
# not from the compiler command line.
handler_deps = deps
handler_cpp_args = []
if uid_button.allowed()
    handler_deps += dependency('pam', required: true)
    if get_option('expire-password')
        handler_cpp_args += '-DEXPIRE_PASSWORD'
    endif
endif

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
    cpp_args: handler_cpp_args,
    implicit_include_directories: true,
    include_directories: ['inc'],
    dependencies: handler_deps,
    install: true,
    install_dir: get_option('bindir'),
)

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_variable(
    pkgconfig: 'systemdsystemunitdir',
    pkgconfig_define: ['prefix', get_option('prefix')],
)

configure_file(
    input: 'service_files/phosphor-button-handler.service',
    output: 'phosphor-button-handler.service',
    copy: true,
    install_dir: systemd_system_unit_dir,
)

configure_file(
    input: 'service_files/xyz.openbmc_project.Chassis.Buttons.service',
    output: 'xyz.openbmc_project.Chassis.Buttons.service',
    copy: true,
    install_dir: systemd_system_unit_dir,
)

# PAM stack used by the UID button password reset.
if uid_button.allowed()
    install_data(
        'service_files/passwordreset',
        install_dir: get_option('sysconfdir') / 'pam.d',
    )
endif
