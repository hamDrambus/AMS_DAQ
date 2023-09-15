from setuptools import find_packages, setup

setup(
    name='daqConfig',
    version='1.0.0',
    packages=["daqConfig"],
    include_package_data=True,
    zip_safe=False,
    install_requires=[
        'flask',
    ],
)
