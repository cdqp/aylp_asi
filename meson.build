project(
	'aylp_asi',
	'c',
	default_options: ['c_std=gnu17', 'warning_level=3', 'optimization=3']
	# this is worth occasionally turning on to check
)

incdir = include_directories(['libasi/include', 'libaylp'])

asipath = meson.source_root()
if target_machine.cpu_family() == 'x86_64'
	asipath = join_paths(asipath, 'libasi/lib/x64')
elif target_machine.cpu_family() == 'arm64'
	asipath = join_paths(asipath, 'libasi/lib/armv8')
else
	error('Unsupported architecture', target_machine.cpu_family())
endif

c = meson.get_compiler('c')
asi_dep = c.find_library(
	'ASICamera2',
	dirs: asipath,
	required: true
)
gsl_dep = dependency('gsl')
json_dep = dependency('json-c')
deps = [asi_dep, gsl_dep, json_dep]

shared_library('aylp_asi', 'aylp_asi.c',
	name_prefix: '',
	dependencies: deps,
	include_directories: incdir,
	install: true,
	install_dir: '/opt/anyloop',
	override_options: 'b_lundef=false'
)

