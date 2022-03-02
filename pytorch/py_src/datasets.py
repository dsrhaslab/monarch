from PIL import Image
from typing import Any, Callable, cast, Dict, List, Optional, Tuple
from torch.utils.data import Dataset
import _py_pastor as middleware
import io
import torch

def tensor_loader(path: str):
    return torch.load(path)

def pil_middleware_loader(dataset_reader, index : int):
    target, byts = dataset_reader.read(index)
    if len(byts) == 0:
        print("Error on index: ", index)
    img = Image.open(io.BytesIO(byts))
    return target, img.convert('RGB')

def pil_loader(path: str) -> Image.Image:
    with open(path, 'rb') as f:
        img = Image.open(f)
        return img.convert('RGB')

class EpochShuffleImageFolder(Dataset):
    def __init__(
            self,
            control_server_address: str,
            multiprocessing: bool,
            multigpu: bool,
            bootstrap_client: middleware.BootstrapClient,
            rank: int,
            train_transform: Optional[Callable] = None,
            val_transform: Optional[Callable] = None,
    ):
        self.group = bootstrap_client.get_group()
        self.control_server_address = control_server_address
        self.indexes = bootstrap_client.get_ids_from_rank(0)
        self.train_transform = train_transform
        self.val_transform = val_transform
        self.loader=pil_middleware_loader
        self.rank = rank

        if multiprocessing:
            self.dataset_reader = None
        elif multigpu:
            self.start_client(0)
        else:
            self.start_data_plane(0)

        #data_source_infos comes in the form : List[Tuple[str : int]] -> converting to Dict[std : int]
        self.data_source_info = {}
        for dir_info in bootstrap_client.get_data_source_infos():
            self.data_source_info[dir_info[0]] = abs(dir_info[1])

    def __getitem__(self, index: int) -> Tuple[Any, Any]:
        target, sample = self.loader(self.dataset_reader, index)
        if index < self.data_source_info["train"]:
            if self.train_transform is not None:
                sample = self.train_transform(sample)
        else:
            if self.val_transform is not None:
                sample = self.val_transform(sample)
        
        return sample, target

    def __len__(self) -> int:
        return len(self.samples)

    def start_client(self, worker_id):
        self.dataset_reader = middleware.USClient()
        print("Worker: ", worker_id, " starting us_client. Server addr: ", self.control_server_address)
        port = str(20000 + worker_id)
        self.dataset_reader.bind(0, self.group, self.control_server_address, "0.0.0.0:" + port)

    #TODO port doesnt work with distributes
    def start_data_plane(self, worker_id):
        self.dataset_reader = middleware.DataPlane(self.rank, worker_id)
        print("Worker: ", worker_id, " starting data plane. Server addr: ", self.control_server_address)
        port = str(20000 + worker_id)
        self.dataset_reader.bind(self.group, self.control_server_address, "0.0.0.0:" + port)
        self.dataset_reader.start()
