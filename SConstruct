from os.path import join

# Adds an option to specify the build directory, by default 'build'.  This
# automatically prevents unintentional in-source builds.
AddOption('--build',
      type='string',
      nargs=1,
      default='build',
      metavar='DIR',
      help='Override directory used for build artifacts')



# Build for pav binary
env = Environment(BUILDDIR=GetOption('build'),
                  CCFLAGS='-g -O3 -fdiagnostics-color -fopenmp',
                  LINKFLAGS='-fopenmp -lgomp')
env.SConscript('pav.scons',
                variant_dir=join(GetOption('build'), 'bin'),
                src_dir='src',
                duplicate=False,
                exports='env')


# Build for pav unit tests
# ut_env = Environment(BUILDDIR=GetOption('build'),
#                      CCFLAGS='-g -O0 -fdiagnostics-color -fopenmp -fprofile-arcs -ftest-coverage',
#                      LINKFLAGS='-fopenmp -lgomp -fprofile-arcs -ftest-coverage -lgtest',
#                      LIBPATH=['/usr/local/lib'],
#                      CPPPATH=['#src', '/usr/local/include'])
# ut_env.SConscript('tests.scons',
#                     variant_dir=join(GetOption('build'), 'tests'),
#                     src_dir='tests',
#                     duplicate=False,
#                     exports='ut_env')
