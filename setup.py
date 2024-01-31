
from distutils.core import setup, Extension

setup(name = 'spike',
      version = '1.0a2',
      author = 'Leif Strand',
      author_email = 'leif.c.strand@gmail.com',
      url = 'http://spike.sourceforge.net/',
      scripts = ['bin/cspk',
                 'bin/spike',
                 ],
      packages = ['spike',
                  'spike.backends',
                  'spike.backends.x86_32',
                  'spike.compiler',
                  'spike.tools',
                  ],
      package_data = {'spike': ['*.spk', 'rtl/*']},
      ext_modules = [Extension('spike.ext',
                               ['ext.c',
                                'gram.c',
                                'lexer.c',
                                ],
                               #undef_macros = ['NDEBUG'],
                               )
                     ],
      )
