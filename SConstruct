import os.path

# Adds an option to specify the build directory, by default 'build'.  This
# automatically prevents unintentional in-source builds.
AddOption('--build',
      type='string',
      nargs=1,
      default='build',
      metavar='DIR',
      help='Override directory used for build artifacts')

#-fdiagnostic-color
env = Environment(BUILDDIR=GetOption('build'),
                  CCFLAGS='-g -O0 -fdiagnostics-color')

obj_dir = os.path.join(GetOption('build'), '')
env.SConscript('pav.scons',
    variant_dir=obj_dir,
    src_dir='src',
    duplicate=False,
    exports='env')
