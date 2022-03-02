from torch.utils.data.sampler import Sampler
from datasets import EpochShuffleImageFolder

class PastorSampler(Sampler):

    def __init__(self, data_source: EpochShuffleImageFolder):
        self.indexes = data_source.indexes
        self.data_source_info = data_source.data_source_info
        self.current_epoch = 0
        #train = 0, val = 1
        self.current_phase = 0

    def __iter__(self):
        if self.current_phase == 0:
            res_list = self.indexes[self.current_epoch * self.data_source_info["train"]: (self.current_epoch + 1) * self.data_source_info["train"]]
            self.current_phase = 1
        else:
            res_list = self.indexes[self.current_epoch * self.data_source_info["val"]: (self.current_epoch + 1) * self.data_source_info["val"]]
            self.current_phase = 0
            self.current_epoch += 1
            
        return iter(res_list)

    def __len__(self) -> int:
        if self.current_phase == 0:
            return self.data_source_info["train"]
        else:
            return self.data_source_info["val"]
