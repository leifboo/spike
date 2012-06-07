
from distutils.core import setup, Extension

setup(name = 'spike',
      version = '1.0a2',
      author = 'Leif Strand',
      author_email = 'leif.c.strand@gmail.com',
      url = 'http://spike.sourceforge.net/',
      packages = ['spike',
                  'spike.compiler',
                  ],
      ext_modules = [Extension('spike.ext',
                               ['ext.c',
                                'gram.c',
                                'lexer.c',
                                ])
                     ],
      )
