import _py_pastor as pastor
import sys

if __name__ == "__main__":
    controller = pastor.ControlApplication()
    controller.run(sys.argv[1])