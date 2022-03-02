#!/bin/bash

#change these variables
# ======================================================================================
ROOT_DIR="${HOME}/maypaper"
REPO="thesis"
WORKSPACE=$ROOT_DIR/$REPO

LIBS_DIR="${WORKSPACE}/pastor/third_party"
SCRIPT_DIR="${WORKSPACE}/pytorch/py_src"
VENV_DIR="${ROOT_DIR}/pastor-venv"
RESOURCES_DIR="${WORKSPACE}/pytorch/resources"
RESULTS_DIR="${ROOT_DIR}/results/pytorch"
SERVER_ADDRESS="0.0.0.0:50051"

CUDA_VER="10.1"
# ======================================================================================

# Default values
DATASET_DIR=""
MODEL="alexnet"
BATCH_SIZE=64
EPOCHS=1
PREFETCH_FACTOR=0
NUM_WORKERS=0
USE_PASTOR=false
NUM_GPUS=4
RUN_DIR="test"
USE_LD_PRELOAD=false

# Functions
function export-vars {
	export LD_LIBRARY_PATH="${LIBS_DIR}/tbb/lib:$LD_LIBRARY_PATH"
}


function monitor {
	sh $RESOURCES_DIR/iostat-csv/iostat-csv.sh > $RUN_DIR/iostat.csv &
	nvidia-smi --query-gpu=utilization.gpu,utilization.memory,memory.total,memory.free,memory.used --format=csv -l 1 -f $RUN_DIR/nvidia-smi.csv &
    if [[ ! -z $COLLECT_IOPS ]]; then
		$RESOURCES_DIR/collect_lustre_stats.sh -r 5 -o $RUN_DIR -s llite -f scratch1 > /dev/null &
	fi
}

function train-model {
    if [ "$MODEL" == "resnet50" -o "$MODEL" == "lenet" -o "$MODEL" == "alexnet" ]
    then 
        if [ "$USE_PASTOR" = true ]
        then 
            echo -e "Model: $MODEL\nDataset: ImageNet\nBatch size: $BATCH_SIZE\n#Epochs: $EPOCHS \nGPU: yes \nFramework: Pytorch\nWORKERS: ${NUM_WORKERS}\nPREFETCH_FACTOR: ${PREFETCH_FACTOR} \nPastor: ENABLED \nDataset: ${DATASET_DIR}" > $RUN_DIR/info.txt
            python3 $SCRIPT_DIR/imagenet_execution.py --arch $MODEL --mw --epochs $EPOCHS --batch-size $BATCH_SIZE --workers $NUM_WORKERS --prefetch-factor $PREFETCH_FACTOR --control-server-address $SERVER_ADDRESS $DATASET_DIR |& tee $RUN_DIR/log.txt
            #python3 $SCRIPT_DIR/imagenet_execution.py --arch $MODEL --mw --epochs $EPOCHS --batch-size $BATCH_SIZE --workers $NUM_WORKERS --prefetch-factor $PREFETCH_FACTOR --dist-url 'tcp://127.0.0.1:1260' --dist-backend 'nccl' --multiprocessing-distributed  --world-size 1 --rank 0 --control-server-address $SERVER_ADDRESS $DATASET_DIR |& tee $RUN_DIR/log.txt
        else
            echo -e "Model: $MODEL\nDataset: ImageNet\nBatch size: $BATCH_SIZE\n#Epochs: $EPOCHS\nGPU: yes\nFramework: Pytorch\nWORKERS: ${NUM_WORKERS}\nPREFETCH_FACTOR: ${PREFETCH_FACTOR} \nPastor: DISABLED \nDALI_ARGS: ${DALI_ARGS} \nDataset: ${DATASET_DIR} \nINDEX_ARGS: ${INDEX_ARGS}" > $RUN_DIR/info.txt
            unbuffer python3 $SCRIPT_DIR/imagenet_execution_dali.py $DIST_ARGS $DALI_ARGS $INDEX_ARGS --arch $MODEL --workers $NUM_WORKERS --epochs $EPOCHS --batch-size $BATCH_SIZE $DATASET_DIR |& tee $RUN_DIR/log.txt
        fi
    else 
	echo "Select a valid configuration. Run train-model -h to see the available options"
    fi
}

function kill-monitor {
	echo -e "\nKilling jobs: \n$(jobs -p)"
	kill $(jobs -p)
	echo -e $(pgrep iostat)
	kill $(pgrep iostat)
}

source $VENV_DIR/bin/activate

# Clean cache and checkpointing directory
#clean

# Update env vars
export-vars

# Handle flags
echo -e "\nHandling flags..."
while getopts ":hm:b:e:g:pxf:i:w:r:d:tno" opt; do
    case $opt in
        h)
            echo "options:"
            echo "-h       show brief help"
            echo "-m       specify the model to train (lenet, resnet or alexnet)"
            echo "-b       specify batch size"
            echo "-e       specify number of epochs"
            echo "-p       use pastor"
            echo "-n       use NVIDIA DALI"
            echo "-f       specify prefetch factor"
            echo "-w       specify numbers of workers"
            echo "-d       specify dataset absolute path"
            echo "-i       specify indexes directory to use DALI with tfrecords"
            echo "-t       use multiprocessing distributed training"
            echo "-o       collect lustre IOPS"
            exit 0
            ;;
        m)
            echo "-m was triggered, MODEL: $OPTARG" >&2
            MODEL=$OPTARG
            ;;
        b)
            echo "-b was triggered, BATCH_SIZE: $OPTARG" >&2
            BATCH_SIZE=$OPTARG
            ;;
        e)
            echo "-e was triggered, EPOCHS: $OPTARG" >&2
            EPOCHS=$OPTARG
            ;;
        g)
            echo "-g was triggered, NUM_GPUS: $OPTARG" >&2
            NUM_GPUS=$OPTARG
            ;;
        p)
            echo "-p was triggered, Middleware is enabled" >&2
            USE_PASTOR=true
            ;;
        f)
            echo "-f was triggered, PREFETCH_FACTOR: $OPTARG" >&2
            PREFETCH_FACTOR=$OPTARG
            ;;
        w)
            echo "-w was triggered, NUM_WORKERS: $OPTARG" >&2
            NUM_WORKERS=$OPTARG
            ;;
        i)
            echo "-i was triggered, INDEX_DIR: $OPTARG" >&2
            INDEX_ARGS="--tfrecords-idx $OPTARG"
            ;;
        d)
            echo "-d was triggered, DATASET_DIR: $OPTARG" >&2
            DATASET_DIR=$OPTARG
            ;;
        n)
            echo "-n was triggered, NVIDIA DALI is enable" >&2
            DALI_ARGS="--use-dali"
            ;;
        o)
            echo "-o was triggered, collect lustre IOPS is enable" >&2
            COLLECT_IOPS=true
            ;;
        t)
            echo "-t was triggered, multiprocess distributed training is enable" >&2
            DIST_ARGS="--dist-url tcp://127.0.0.1:4321 --dist-backend nccl --multiprocessing-distributed --world-size 1 --rank 0"
	        ;;
        r)
            echo "-r was triggered, run dir: $OPTARG" >&2
            RUN_DIR=$OPTARG
            ;;
        x)
            echo "-x was triggered, Monarch is enabled through ld_preload"
            USE_LD_PRELOAD=true
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            exit 1
            ;;
    esac
done


# Create results directory
mkdir -p $RUN_DIR

# Create log file
touch $RUN_DIR/log.txt

# Update data loader 
#update-data-loader

# Start monitoring tools
trap 'kill 0' SIGINT # trap to kill all bg processes when pressing CTRL-C
monitor
sleep 10

# Start training the model
SECONDS=0

if [ "$USE_LD_PRELOAD" = true ]
then
  MONARCH_DIR="${HOME}/maypaper/thesis/pastor/build/libmonarch.so"
  LD_PRELOAD=$MONARCH_DIR train-model
else
  train-model
fi

echo "ELAPSED TIME: $SECONDS s" | tee -a $RUN_DIR/log.txt
sleep 10

# Kill monitor process
kill-monitor
