# Hello

This is an experiment that might turn into a some DSP code for a Eurorack
module, initially targetting Teensy 4. It's built as a Zephyr application.

# Building the code

This repo is a manifest repo for Zephyr, using a "T2" topology. Zephyr's
management tool `west` should be used to check out the needed repos.

Follow Zephyr's getting started guide to install dependencies and create a
Python virtual environment. Then check out this repo with `west`. Something
like this:

```
mkdir eurorack-dsp
cd eurorack-dsp
python3 -m venv .venv

# Activate the virtual environment (this you have to do in every new shell)
source .venv/bin/activate

pip install west

# Initialize a west workspace from this repo
west init -m https://github.com/JonasNorling/eurorack-dsp.git

# Download the Zephyr stack
west update

# Install Python packages
west packages pip --install

# Install C compilers
west sdk install
```

Build thus:
```
cd eurorack-dsp
west build
```
