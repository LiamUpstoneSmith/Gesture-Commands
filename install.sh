sudo dnf install zeromq zeromq-devel
sudo dnf install cppzmq-devel
sudo dnf install gcc-c++
sudo dnf install glfw
sudo dnf install glfw-devel

python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
