import io
import os
import random
import _py_pastor as middleware
from multiprocessing import Process
from PIL import Image
import time

def run_server():
    middleware.USServer().start()

def test(rank, control_server_address):
    p = Process(target=run_server, args=())
    p.daemon = True
    p.start()
    time.sleep(3)
    bootstrap_client = middleware.BootstrapClient(control_server_address)
    application_id = "pytorch" + str(random.randint(0, 10000))
    bootstrap_client.request_session("multi_process", application_id, 1, 1, False)
    group = bootstrap_client.get_group()
    indexes = bootstrap_client.get_ids_from_rank(rank)

    client = middleware.USClient()
    print("Worker: ", 0, "binding to controller at: ", control_server_address)
    port = str(20000)
    client.bind(rank, group, control_server_address, "0.0.0.0:" + port)

    for i in range(10):
        target, byts = client.read(indexes[i])
        print("target: ", target)
        img = Image.open(io.BytesIO(byts))
        img.show()
        time.sleep(10)

def test2(rank, control_server_address):
    bootstrap_client = middleware.BootstrapClient(control_server_address)
    application_id = "pytorch" + str(random.randint(0, 10000))
    bootstrap_client.request_session("multi_process", application_id, 1, 1, False)
    group = bootstrap_client.get_group()
    indexes = bootstrap_client.get_ids_from_rank(rank)

    client = middleware.DataPlane(rank, 0)
    print("Worker: ", 0, "binding to controller at: ", control_server_address)
    port = str(20000)
    client.bind(group, control_server_address, "0.0.0.0:" + port)
    client.start()
    for i in range(1,10):
        target, byts = client.read(indexes[i])
        print("target: ", target)
        img = Image.open(io.BytesIO(byts))
        img.show()
        time.sleep(10)

if __name__ == "__main__":
    test(0, "0.0.0.0:50051")



