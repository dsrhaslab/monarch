from os import listdir
from os.path import isfile, join
import re
import random
import os
import sys
import getopt


def get_filenames(data_dir, regex):
    # Create list with the training filenames
    return [f for f in listdir(data_dir) if match_file(data_dir, f, regex)]


def match_file(data_dir, file, regex):
    # Match file with regex
    match = re.search(regex, file)

    # Check if file exists and matches regex
    if isfile(join(data_dir, file)) and match:
        return True

    else:
        return False


def get_filenames_list_path():
    # Get HOME environment var
    home = os.environ['HOME']
    # Get PRISMA directory path
    prisma_dir = home + "/prisma"

    # Create PRISMA directory if necessary
    if not os.path.exists(prisma_dir):
        print("PRISMA directory " + prisma_dir + " created.")
        os.makedirs(prisma_dir)

    return prisma_dir + "/filenames_list"


# Shuffle filename indexes
def shuffle_indexes(file_mappings, num_epochs):
    indexes = list(file_mappings.keys())

    # Open filenames list
    filenames_list_path = get_filenames_list_path()
    filenames_list_file = open(filenames_list_path, "w")
    indexes_list = []

    # Repeat for the number of epochs
    for e in range(num_epochs):

        # Shuffle indexes
        random.shuffle(indexes)

        # Write filename
        for i in indexes:
            indexes_list.append(i)
            _, filename, _ = file_mappings[i]
            filenames_list_file.write("%s\n" % filename)

    print("File " + filenames_list_path + " created.")

    # Close out file
    filenames_list_file.close()

    # Return indexes list
    return indexes_list


# Shuffle filenames list
def shuffle_filenames(filenames, num_epochs):
    # Open filenames list
    filenames_list_path = get_filenames_list_path()
    filenames_list_file = open(filenames_list_path, "w")
    filenames_list = []

    # Repeat for the number of epochs
    for i in range(num_epochs):
        # Shuffle filenames
        random.shuffle(filenames)

        # Write filename
        for f in filenames:
            filenames_list_file.write("%s\n" % f)
            # Append filename to filenames list
            filenames_list.append(f)

    print("File " + filenames_list_path + " created.")

    # Close out file
    filenames_list_file.close()

    # Return filenames list
    return filenames_list


# Get filenames list and shuffle it
def shuffle(data_dir, regex, num_epochs):
    # Get filenames list
    filenames = get_filenames(data_dir, regex)

    # Check if filenames list is empty
    if not filenames:
        print("No filenames found in" + data_dir + "that match regex " + regex + ".")

    # Open out file
    filenames_list_path = get_filenames_list_path()
    filenames_list_file = open(filenames_list_path, "w")
    filenames_list = []

    # Repeat for the number of epochs
    for i in range(num_epochs):

        # Shuffle filenames
        random.shuffle(filenames)

        # Write filename
        for f in filenames:
            file_path = data_dir + "/" + f
            filenames_list_file.write("%s\n" % file_path)
            # Append filename to train_list
            filenames_list.append(file_path)

    print("File " + filenames_list_path + " created.")

    # Close out file
    filenames_list_file.close()

    # Return filenames list
    return filenames_list


def main(argv):
    data_dir = ''
    regex = ''
    epochs = 0

    try:
        opts, args = getopt.getopt(argv, "hd:r:e:", ["data_dir=", "regex=", "epochs="])
    except getopt.GetoptError:
        print('shuffle_filenames.py -d <data_dir> -r <regex> -e <num_epochs>')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print('shuffle_filenames.py -d <data_dir> -r <regex> -e <num_epochs>')
            sys.exit()
        elif opt in ("-d", "--data_dir"):
            data_dir = arg
        elif opt in ("-r", "--regex"):
            regex = arg
        elif opt in ("-e", "--epochs"):
            epochs = int(arg)

    shuffle(data_dir, regex, epochs)


if __name__ == "__main__":
    main(sys.argv[1:])
