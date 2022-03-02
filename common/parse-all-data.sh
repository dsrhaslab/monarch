#!/bin/bash

RED='\033[1;31m'
NC='\033[0m'
SPACE="  "
SUBSPACE="      "
STDOUT="/dev/null"

if [[ "$OSTYPE" == "darwin"* ]]; then
  shopt -s expand_aliases
  alias sed=gsed
fi

function print-info() {
  echo "${SPACE}==> $@" > "${STDOUT}"
}

function print-error() {
  echo -e "${RED}* Error:${NC} $@"
}

function print-sub-error() {
  echo -e "${SUBSPACE}${RED}* Error:${NC} $@"
}

function print-sub-info() {
  echo "${SUBSPACE}+ $@" > "${STDOUT}"
}

function print-bullet() {
  echo "${SUBSPACE}> $@" > "${STDOUT}"
}

function usage() {
  echo "Usage: $0 -d input_directory [-o output_directory] [-v]" 
  exit 1
}

function clean-info() {
  print-info "Cleaning info data... "

  file=$(find $DATADIR -name "info.txt")

  if [[ -z "$file" ]]; then 
    print-sub-error "file not found!"
  else 
    # Clean info data
    OUTFILE="$OUTDIR/info.txt"
    cat $file > ${OUTFILE}
    print-sub-info "clean data written in ${OUTFILE}"
  fi

  unset file
}

function clean-log() {
  print-info "Cleaning log data... "

  file=$(find $DATADIR -name "log.txt")
  info_file=$(find $DATADIR -name "info.txt")


  if [[ -z "$file" || -z "$info_file" ]]; then 
    print-sub-error "file not found!"
  else 
    if grep -iq "tensorflow" "$info_file"; then 
      # Clean log data
      OUTFILE="$OUTDIR/log.txt"
      grep "accuracy:" $file |
      awk 'BEGIN  { 
                    FS="-|:"; RS="\n"; epoch=1; 
                    print "epoch time_s loss accuracy" 
                  } 
                  { gsub("s", "", $2); print epoch++, $2, $4, $6 }' |
      column -t > ${OUTFILE}
      print-sub-info "clean data written in ${OUTFILE}"

      # Write accuracy file
      OUTFILE_ACC="$OUTDIR/accuracy.txt"
      awk 'NR==1 {print $4}; END {print $4}' ${OUTFILE} > ${OUTFILE_ACC}
      print-sub-info "accuracy data written in ${OUTFILE_ACC}"

      # Clean train time data
      OUTFILE_TRAIN_TIME="$OUTDIR/train-time.txt"
      echo "training_time_s" > ${OUTFILE_TRAIN_TIME}
      grep -o "ELAPSED TIME:.*" $file | cut -d ":" -f 2 | tr -d " s" >> ${OUTFILE_TRAIN_TIME}
      print-sub-info "train time data written in ${OUTFILE_TRAIN_TIME}"

      # Clean steps data
      OUTFILE_STEPS="$OUTDIR/steps.txt"
      grep -o "{'global step.*" $file | tr -d "{}' " | tr "," ":" | cut -d ":" -f 2,4,6 | tr ":" " " |
      sed "1s/^/global_step time_taken examples_per_second\n/" | column -t > ${OUTFILE_STEPS}
      print-sub-info "steps data written in ${OUTFILE_STEPS}"

    elif grep -iq "pytorch" "$info_file"; then 
      OUTFILE="$OUTDIR/log.txt"
      grep -E "(\*|EPOCH)" "$file" | 
      sed "s/ \* //g" | 
      awk 'BEGIN  {
                    FS=" |\n"; RS="s\n"; epoch=1; 
                    print "epoch time_s accuracy_1 accuracy_5"
                  } 
                  { print epoch++, $8, $2, $4 }' | 
      column -t > ${OUTFILE}
      print-sub-info "clean data written in ${OUTFILE}"

      # Write accuracy file 
      OUTFILE_ACC="$OUTDIR/accuracy.txt"
      awk 'NR==1 {print $3, $4}; END {print $3, $4}' ${OUTFILE} | column -t > ${OUTFILE_ACC}
      print-sub-info "accuracy data written in ${OUTFILE_ACC}"

      # Clean train time data
      OUTFILE_TRAIN_TIME="$OUTDIR/train-time.txt"
      echo "training_time_s" > ${OUTFILE_TRAIN_TIME}
      grep -E "Total time" $file | cut -d "=" -f 2 | tr -d " s" >> ${OUTFILE_TRAIN_TIME}
      print-sub-info "train time data written in ${OUTFILE_TRAIN_TIME}"

      # Clean steps data
      OUTFILE_STEPS="$OUTDIR/steps.txt"
      grep -E "Epoch:" $file | 
      sed 's/( */(/g' | sed 's/\[ */\[/g' | tr "\t" " " | tr -s " " |
      awk 'BEGIN {step=1; print "step time avg_time data avg_data loss avg_loss acc_1 avg_acc_1 acc_5 avg_acc_5"} {print step++, $4, $5, $7, $8, $10, $11, $13, $14, $16, $17}' |
      sed -E 's/[\(\)]//g' | column -t > ${OUTFILE_STEPS}
      print-sub-info "steps data written in ${OUTFILE_STEPS}"
    fi

  fi

  unset file
  unset info_file
}

function clean-remora-cpu() {
  print-info "Cleaning remora CPU data... "

  file=$(find $DATADIR -name "cpu*.txt")

  if [[ -z "$file" ]]; then 
    print-sub-error "file not found!"
  else 
    # Clean remora-cpu data
    OUTFILE="$OUTDIR/remora-cpu.txt"
    cat $file | 
    sed -E "s/^ *//g"| 
    cut -d " " -f 1-2 | 
    sed -E "s/(%time)/#\n\1/g" |
    awk 'BEGIN  {RS="#\n"; FS="\n"; OFS=" "; ORS="\n"} 
                {print $1,$2,$3,$4}' |
    cut -d " " -f 2,4,6,8 | 
    sed -E "1c\#TIME %usr %sys %gnice" |
    column -t > ${OUTFILE}
    print-sub-info "clean data written in ${OUTFILE}"

    # Average remora-cpu utilization
    AVG_OUTFILE="$OUTDIR/avg-remora-cpu.txt"
    cat ${OUTFILE} |
    awk 'NR==1  { $1=""; header=$0 }
                { for(i=2; i<=NF; i++){ sum[i]+= $i } }
         END    { 
                  print(header);
                  if(NR>1) {
                    for(i=2; i<=NF; i++){ printf("%.2f ", sum[i]/(NR-1)) } 
                    printf("\n");
                  } 
                }' | 
    column -t > ${AVG_OUTFILE}
    print-sub-info "average remora-cpu utilization written in ${AVG_OUTFILE}"
  fi

  unset file
}

function clean-remora-io() {
  print-info "Cleaning remora IO data... "

  file=$(find $DATADIR -name "lustre_stats_*.txt")

  if [[ -z "$file" ]]; then 
    print-sub-error "file not found!"
  else 
    # Clean remora-io data
    OUTFILE="$OUTDIR/remora-io-rate.txt"
    cat $file |
    tr -s " " " " |
    awk 'NR==1  { print }
         NR==2  { t0=$1; for(i=1;i<=NF;i++) last_line[i]=$i } 
         NR>2   { 
                  dt=$1-last_line[1]; last_line[1]=$1; $1=$1-t0;
                  for(i=2; i<=NF; i++){ tmp=$i; $i=($i-last_line[i])/dt; last_line[i]=tmp }
                  for(i=2; i<=NF; i++){ if ((i-2)%3!=0) $i=$i/1000/1000 } 
                  printf("%s ", $1); 
                  for(i=2; i<=NF; i++){ printf("%.6f ", $i) }
                  printf("\n");
                }' |
    sed -E "1s/(RD|WR)/\1-MB\/s/g" | 
    sed -E "1s/(RQ)/\1-IOPS/g" |
    column -t > ${OUTFILE}
    print-sub-info "clean data written in ${OUTFILE}"

    # Average remora-io utilization
    AVG_OUTFILE="$OUTDIR/avg-remora-io-rate.txt"
    cat ${OUTFILE} |
    awk 'NR==1  { $1=""; header=$0 }
                { for(i=2; i<=NF; i++){ sum[i]+= $i } }
         END    { 
                  print(header);
                  if(NR>1) {
                    for(i=2; i<=NF; i++){ printf("%.6f ", sum[i]/(NR-1)) } 
                    printf("\n");
                  } 
                }' | 
    column -t > ${AVG_OUTFILE}
    print-sub-info "average remora-io utilization written in ${AVG_OUTFILE}"
  fi

  unset file
}


function clean-remora-memory() {
  print-info "Cleaning remora memory data... "

  file=$(find $DATADIR -name "memory_stats*.txt")

  if [[ -z "$file" ]]; then 
    print-sub-error "file not found!"
  else 
    # Clean remora-memory data
    OUTFILE="$OUTDIR/remora-memory.txt"
    cat $file |
    sed -E "1s/( [A-Z_]+)/\1_GiB/g" |
    column -t > ${OUTFILE}
    print-sub-info "clean data written in ${OUTFILE}"

    # Average remora-memory utilization
    AVG_OUTFILE="$OUTDIR/avg-remora-memory.txt"
    cat ${OUTFILE} |
    awk 'NR==1  { $1=""; header=$0 }
                { for(i=2; i<=NF; i++){ sum[i]+= $i } }
         END    { 
                  print(header);
                  if(NR>1){
                    for(i=2; i<=NF; i++){ printf("%.6f ", sum[i]/(NR-1)) } 
                    printf("\n");
                  } 
                }' | 
    column -t > ${AVG_OUTFILE}
    print-sub-info "average remora-memory utilization written in ${AVG_OUTFILE}"
  fi

  unset file 
}

function clean-remora-gpu-memory() {
  print-info "Cleaning remora gpu memory data... "

  file=$(find $DATADIR -name "gpu_memory_stats*.txt")

  if [[ -z "$file" ]]; then 
    print-sub-error "file not found!"
  else
    # Clean remora-gpu-memory data
    OUTFILE="$OUTDIR/remora-gpu-memory.txt"
    cat $file | 
    awk 'NR==1 { print $0, "all_gpu_avail_GB", "all_gpu_used_GB" }; 
         NR>1  { print $0, sprintf("%.3f", $2+$4+$6+$8), sprintf("%.3f",$3+$5+$7+$9) }' |
    column -t > ${OUTFILE}
    print-sub-info "clean data written in ${OUTFILE}"

    # Average remora-gpu-memory utilization
    AVG_OUTFILE="$OUTDIR/avg-remora-gpu-memory.txt"
    cat ${OUTFILE} |
    awk 'NR==1  { $1=""; header=$0 }
                { for(i=2; i<=NF; i++){ sum[i]+= $i } }
         END    { 
                  print(header);
                  if(NR>1){ 
                    for(i=2; i<=NF; i++){ printf("%.3f ", sum[i]/(NR-1))} 
                    printf("\n");
                  }
                }' | 
    column -t > ${AVG_OUTFILE}
    print-sub-info "average remora-gpu-memory utilization written in ${AVG_OUTFILE}"
  fi

  unset file
}

function clean-nvidia-smi() {
  print-info "Cleaning nvidia-smi data..."

  file=$(find $DATADIR -name "nvidia-smi.csv")

  if [[ -z "$file" ]]; then 
    print-sub-error "file not found!"
  else 
    # Clean nvidia-smi data
    OUTFILE="$OUTDIR/nvidia-smi.txt"
    cat $file |
    sed -E "1c\utilization_gpu_% utilization_memory_% memory_total_MiB memory_free_MiB memory_used_MiB" |
    sed -E 's/ (%|MiB)//g' |  
    tr -d "," | 
    column -t > ${OUTFILE}
    print-sub-info "clean data written in ${OUTFILE}"

    # Average GPU utilization
    AVG_OUTFILE="$OUTDIR/avg-nvidia-smi.txt"
    cat ${OUTFILE} |
    awk 'NR==1  { header=$0 }
                { for(i=1; i<=NF; i++){ sum[i]+= $i } }
         END    { 
                  print(header);
                  if(NR>1){
                    for(i=1; i<=NF; i++){ printf("%.2f ", sum[i]/(NR-1))} 
                    printf("\n");
                  }
                }' | 
    column -t > ${AVG_OUTFILE}
    print-sub-info "average GPU utilization written in ${AVG_OUTFILE}"
  fi

  unset file
}

function clean-iostat() {
  print-info "Cleaning iostat data... "

  file=$(find $DATADIR -name "iostat.csv")

  if [[ -z "$file" ]]; then 
    print-sub-error "file not found!"
  else 
    # Clean iostat CPU data
    OUTFILE_CPU="$OUTDIR/iostat-cpu.txt"
    cat $file |
    cut -d "," -f 2-8 |
    tr "," " " | 
    column -t > ${OUTFILE_CPU}
    print-sub-info "clean data written in ${OUTFILE_CPU}"

    # Average CPU consumption
    AVG_OUTFILE_CPU="$OUTDIR/avg-iostat-cpu.txt"
    cat ${OUTFILE_CPU} |
    awk 'NR==1  { $1=""; header=$0 }
                { for(i=2; i<=NF; i++){ sum[i]+= $i } }
         END    { 
                  print(header); 
                  if(NR>1){ 
                   for(i=2; i<=NF; i++){ printf("%.2f ", sum[i]/(NR-1))} 
                   printf("\n");
                  }
                }' | 
    column -t > ${AVG_OUTFILE_CPU}
    print-sub-info "average CPU consumption written in ${AVG_OUTFILE_CPU}"

    # Clean iostat device utilization data
    OUTFILE_DEV="$OUTDIR/iostat-device.txt"
    cat $file | 
    cut -d "," -f 2,51- | # device dm-2 (/tmp mountpoint)
    tr "," " " | 
    column -t > ${OUTFILE_DEV}
    print-sub-info "clean data written in ${OUTFILE_DEV}"

    # Average device utilization
    AVG_OUTFILE_DEV="$OUTDIR/avg-iostat-device.txt"
    cat ${OUTFILE_DEV} |
    awk 'NR==1  { $1=$2=""; header=$0 }
                { for(i=3; i<=NF; i++){ sum[i]+= $i } }
         END    {  
                  print(header);
                  if(NR>1){ 
                    for(i=3; i<=NF; i++){ printf("%.2f ", sum[i]/(NR-1))} 
                    printf("\n");
                  }
                }' | 
    column -t > ${AVG_OUTFILE_DEV}
    print-sub-info "average device utilization written in ${AVG_OUTFILE_DEV}"
  fi

  unset file
}

function clean-lustre-iops() {
  print-info "Cleaning lustre stats... "

  files=$(find $DATADIR -path "*/lustre_stats/*" -type f)

  if [[ -z "$files" ]]; then 
    print-sub-error "files not found!"
  else 
    for file in $files; do 
      # Accumulated lustre IOPS
      OUTFILE_ACC="$OUTDIR/$(echo "$file" | rev | cut -d "/" -f 1-2 | rev | tr "/" "-" | sed 's/-/-acc-/g')"
      cat $file |
      awk 'NR==1  { print }
           NR==2  { for(i = 2; i <= NF; i++) baseline[i] = $i }
           NR>=2  { for(i = 2; i <= NF; i++) $i = $i - baseline[i]; print $0 }' |
      column -t > ${OUTFILE_ACC}
      print-sub-info "clean data written in ${OUTFILE_ACC}"

      # Clean lustre IOPS
      OUTFILE="$OUTDIR/$(echo "$file" | rev | cut -d "/" -f 1-2 | rev | tr "/" "-")"
      cat $file |
      awk 'NR==1  { print }
           NR==2  { for(i = 2; i <= NF; i++) last[i] = $i }
           NR>=2  { for(i = 2; i <= NF; i++) { value = $i; $i = $i - last[i]; last[i] = value}; print $0 }' |
      column -t > ${OUTFILE}
      print-sub-info "clean data written in ${OUTFILE}"
    done
  fi

  unset files
}

function clean-all() {
  clean-info
  clean-log
  clean-remora-cpu
  clean-remora-io
  clean-remora-memory
  clean-remora-gpu-memory
  clean-nvidia-smi
  clean-iostat
  clean-lustre-iops
}

while getopts ":d:o:hv" opt; do
  case ${opt} in
    d )
      DATADIR_ROOT=$OPTARG
      ;;
    o )
      OUTDIR_ROOT=$OPTARG
      ;;
    h )
      usage
      ;;
    v )
      STDOUT="/dev/stdout"
      ;;
    \? )
      echo "Invalid option: $OPTARG" 1>&2
      usage
      ;;
    : )
      echo "Invalid option: $OPTARG requires an argument" 1>&2
      usage
      ;;
  esac
done

if [[ -z $DATADIR_ROOT ]]; then
  echo "Missing argument -d" 1>&2
  usage
fi

if [[ ! -d $DATADIR_ROOT ]]; then
  echo "$DATADIR_ROOT is not a dir" 1>&2 
  usage
else
  DATADIR_ROOT=$(cd $DATADIR_ROOT && pwd) # Full path
fi

if [[ -z $OUTDIR_ROOT ]]; then
  OUTDIR_ROOT="/tmp/$(basename ${DATADIR_ROOT})-clean-stats"
fi

# Check if there is only one remora directory and all required files exist
function valid_directory() {
  test $(find $DATADIR -name "remora*" -type d | wc -l) -eq 1 \
  && find $DATADIR -name "info.txt" | read \
  && find $DATADIR -name "log.txt" | read \
  && find $DATADIR -name "cpu*.txt" | read \
  && find $DATADIR -name "lustre_stats_*.txt" | read \
  && find $DATADIR -name "memory_stats*.txt" | read \
  && find $DATADIR -name "gpu_memory_stats*.txt" | read \
  && find $DATADIR -name "nvidia-smi.csv" | read \
  && find $DATADIR -name "iostat.csv" | read
}

# Clean the data and calculate the averages of the collected stats per file
for info_file in $(find $DATADIR_ROOT -iname "info*.txt"); do
  DATADIR=$(dirname $info_file)
  SUBPATH=$(dirname ${DATADIR/#$DATADIR_ROOT\/})
  OUTDIR="${OUTDIR_ROOT}/${SUBPATH}/clean-data/$(basename $DATADIR)"

  if valid_directory; then
    mkdir -p $OUTDIR 
    echo "Cleaning data in $DATADIR"
    #echo "Using $OUTDIR as output directory"
    clean-all
  else 
    print-error "Missing required files or too many remora directories in $DATADIR"
  fi
done

# Calculate the mean and standard deviation per column after aggregating the files
function aggregate_files() {
  cat $@ | 
  awk 'NR==1 || NR%2==0' |
  awk 'NR==1  { header="stats "$0 }
       NR>1   { 
                for(f=1; f<=NF; f++) {
                  sum[f] += $f
                  value[NR-1][f] = $f
                }
              }
       END    { 
                print(header);
                if(NR>1){
                  NVALUES=NR-1

                  printf("mean ")
                  for(f=1; f<=NF; f++){ 
                    mean[f] = sum[f]/NVALUES
                    printf("%.6f ", mean[f]) 
                  } 
                
                  printf("\nstdev ")
                  for(f=1; f<=NF; f++){
                    sd = 0
                    for(r=1; r<=NVALUES; r++){
                      sd += (value[r][f] - mean[f])^2
                    }
                    printf("%.6f ", sqrt(sd/NVALUES))
                  }
                  printf("\n")
                }  
              }' |
  column -t
}

# Calculate the mean and standard deviation per column 
function aggregate_files_by_time() {
  local N_COLUMNS=$(awk '{print NF; exit}' $1)
  paste $@ |
  awk -v N_COLUMNS="${N_COLUMNS}" \
  'NR==1  { for(i=N_COLUMNS+1;i<=NF;i++) $i=""
            for(i=2;i<=N_COLUMNS;i++) $i=$i"_mean "$i"_stdev"
            print 
          }
   NR>1   { printf("%s ",$1)
            for(i=2;i<=N_COLUMNS;i++) {
              # Average
              n_values = 0; mean = 0
              for(j=i;j<=NF;j+=N_COLUMNS){
                mean += $j
                n_values++
              }
              mean = mean/n_values
              printf("%.6f ", mean)

              # Standard deviation
              sd = 0
              for(j=i;j<=NF;j+=N_COLUMNS){
                sd += ($j - mean)^2
              }
              printf("%.6f ", sqrt(sd/n_values))
                  
            } 
            printf("\n")
          }' |
  column -t
}

files_to_agg=("avg-iostat-cpu.txt" 
              "avg-iostat-device.txt"
              "avg-nvidia-smi.txt"
              "avg-remora-cpu.txt"
              "avg-remora-gpu-memory.txt"
              "avg-remora-io-rate.txt"
              "avg-remora-memory.txt"
              "train-time.txt"
              "accuracy.txt")

files_to_agg_by_time=("steps.txt"
                      "log.txt")

function name-run() {
    local model=$(grep -i "model:" $1 | tr -d " " | cut -d ":" -f 2 | tr "[:upper:]" "[:lower:]" | cut -d "-" -f 1)
    local bs=$(grep -i "batch size:" $1 | tr -d " " | cut -d ":" -f 2)
    local ep=$(grep -i "epochs:" $1 | tr -d " " | cut -d ":" -f 2)
    local fs=$(grep -i "dataset:" $1 | tail -n 1 | tr -d " " | cut -d ":" -f 2 | cut -d "/" -f 2)
    local dataset=$(grep -i "dataset:" $1 | tail -n 1 | tr -d " " | cut -d ":" -f 2 | xargs basename | tr "_" "-")
    #echo "${model}-bs${bs}-ep${ep}-${dataset}-${fs}"
    echo "aggregate-${model}"
}

for group in $(find ${OUTDIR_ROOT} -type d -path "*/clean-data/*" | rev | cut -d "/" -f 3- | rev | sort -u); do
  for runs in $(find $group -mindepth 1 -path "*/clean-data/*info.txt" -print -exec echo "#" \; -exec cat {} \; -exec echo \; | 
                awk 'BEGIN { RS="\n\n"; FS="\n#\n" } { content[$2] = content[$2]$1":" } END { for (file in content) { print content[file];} }'); 
  do 
    info_files=$(echo $runs | tr ":" " ")
    runs_to_group=$(for f in $info_files; do echo $f | xargs dirname; done)

    info_file=$(echo $info_files | tr -s " " | cut -d " " -f 1) # Pick the first file
    run_name=$(name-run ${info_file})
    OUTDIR="$group/$run_name"
    mkdir -p ${OUTDIR}

    echo "Joining ${run_name} test data. Writting data to ${OUTDIR}"
    for fg in "${files_to_agg[@]}" "${files_to_agg_by_time[@]}"; do
      files=$(find $runs_to_group -iname $fg)
      if [[ -z "$files" ]]; then 
        print-sub-error "$fg files not found!"
      else 
        print-info "Calculating averages and standard deviations of files:"
        for f in $(echo $files | tr "\n" " "); do print-bullet $f; done
        OUTFILE="${OUTDIR}/$(echo $fg | sed "s/avg-//")"
        if [[ "${files_to_agg[@]}" =~ "$fg" ]]; then 
          aggregate_files $files > ${OUTFILE}
        fi
        if [[ "${files_to_agg_by_time[@]}" =~ "$fg" ]]; then 
          aggregate_files_by_time $files > ${OUTFILE}
        fi
        print-sub-info "data written in ${OUTFILE}"
      fi
      unset files
    done
  done
done
