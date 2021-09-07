from tensorflow.python.keras.layers import Conv2D 
from tensorflow.python.keras.layers import AveragePooling2D 
from tensorflow.python.keras.layers import Flatten
from tensorflow.python.keras.layers import Dense
from tensorflow.python.keras.models import Sequential


def lenet():
    model = Sequential()
    model.add(Conv2D(filters=16, input_shape=(224,224,3), kernel_size=(5,5), strides=(1,1), padding='valid', data_format='channels_last'))
    model.add(AveragePooling2D(pool_size=(2,2), strides=(2,2), padding='same', data_format='channels_last'))
    model.add(Conv2D(filters=32, kernel_size=(3,3), strides=(1,1), activation='tanh'))
    model.add(AveragePooling2D(pool_size=(2,2), strides=(2,2), padding='same',data_format='channels_last'))
    model.add(Conv2D(filters=64, kernel_size=(5,5), strides=(1,1), activation='tanh'))
    model.add(Flatten())
    model.add(Dense(120, activation='tanh'))
    model.add(Dense(84, activation='tanh'))
    model.add(Dense(1000, activation='softmax'))

    return model
