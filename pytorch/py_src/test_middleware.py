import pybind11_example as py
import datasets as custom_datasets
import samplers
from torchvision import transforms
from multiprocessing import Process
from updated_data.dataloader import DataLoader


def run_server(dp_server: py.DataPlaneServer):
    dp_server.start()

def worker_init_fn(worker_id):
    print("worker ",worker_id," init fn")
    worker_info = Pytorch.py_src.updated_data._utils.worker.get_worker_info()
    dataset = worker_info.dataset
    dataset.start_client()

#dp_server = py.DataPlaneServer()
#dp_server.init()
#p = Process(target=run_server, args=(args.config_file,dp_server,))
#p.daemon = True
#p.start()

def main():
    #source_dir = "/home/dantas/disk_test_dir/hdd/hymenoptera_data"
    #configs_file = "/home/dantas/Projects/thesis/aux_files/_configs1.yaml"
    #shuffle_file = "/home/dantas/Projects/thesis/aux_files/shuffle_filenames.txt"
    source_dir = "/home/gsd/marco_dantas/imagenet/normal"
    configs_file = "/home/gsd/marco_dantas/PAStor/configurations/test.yaml"
    shuffle_file = "/home/gsd/marco_dantas/output_home/shuffled_filenames.txt"
    num_workers = 8
    epoch_size = 3

    normalize = transforms.Normalize(mean=[0.485, 0.456, 0.406],
                                     std=[0.229, 0.224, 0.225])

    train_transforms = transforms.Compose([
        transforms.RandomResizedCrop(224),
        transforms.RandomHorizontalFlip(),
        transforms.ToTensor(),
        normalize,
    ])

    val_transforms = transforms.Compose([
        transforms.Resize(256),
        transforms.CenterCrop(224),
        transforms.ToTensor(),
        normalize,])

    if num_workers > 0:
        dp_server = py.DataPlaneServer()
        dp_server.create(configs_file)
        p = Process(target=run_server, args=(dp_server,))
        p.daemon = True
        p.start()
        print("Server started")

    dataset = custom_datasets.EpochShuffleImageFolder(source_dir, False, (num_workers > 0),
                                                      False, configs_file,
                                                      shuffle_file, train_transform=train_transforms,
                                                      val_transform=val_transforms)

    sampler = samplers.ShuffleSampler(dataset)
    data_loader = DataLoader(dataset, batch_size=256, sampler=sampler, num_workers=num_workers, pin_memory=True, worker_init_fn=worker_init_fn)

    for e in range(epoch_size):
        print("###### train ######")
        for i, (images, target) in enumerate(data_loader):
            if i % 1000 == 0:
                print(i, " of ", len(data_loader))
            continue

        print("###### val ######")
        for i, (images, target) in enumerate(data_loader):
            if i % 10 == 0:
                print(i, " of ", len(data_loader))
            continue

if __name__ == "__main__":
    main()