from setuptools import setup, find_packages
import os

here = os.path.abspath(os.path.dirname(__file__))
cffi_dep='cffi>=1.1'
setup(name="flux",
      version="0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.1a1",
      description="Bindings to the flux resource manager API",
      setup_requires=[cffi_dep],
      packages=find_packages(),
      cffi_modules=[
          "flux/_core_build.py:ffi",
          "flux/_jsc_build.py:ffi",
          "flux/_kvs_build.py:ffi",
          "flux/_kz_build.py:ffi",
      ],
      install_requires=[cffi_dep],
      )
