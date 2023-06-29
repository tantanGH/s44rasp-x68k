import setuptools

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setuptools.setup(
    name="s44raspd",
    version="0.5.1",
    author="tantanGH",
    author_email="tantanGH@github",
    license='MIT',
    description="backend service daemon for s44rasp over UART",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/tantanGH/s44rasp-x68k",
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    entry_points={
        'console_scripts': [
            's44raspd=s44raspd.s44raspd:main'
        ]
    },
    packages=setuptools.find_packages(),
    python_requires=">=3.7",
    setup_requires=["setuptools"],
    install_requires=["pyserial"],
)
