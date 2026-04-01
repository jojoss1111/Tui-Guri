from setuptools import setup, Extension
from Cython.Build import cythonize

ext = Extension(
    name="tui",
    sources=["tui.pyx", "tui_api.c"],
    include_dirs=["."],
    extra_compile_args=["-std=c11", "-O2", "-Wall"],
)

setup(
    name="tui",
    ext_modules=cythonize(
        [ext],
        compiler_directives={"language_level": "3"},
    ),
)