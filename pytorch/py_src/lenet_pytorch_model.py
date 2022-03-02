import torch
import torch.nn as nn
import torch.nn.functional as F


## Assumes input (3 x 224 x 224)

class LeNet(nn.Module):
    def __init__(self, num_classes=1000):
        super(LeNet, self).__init__()
        self.features = nn.Sequential(
            nn.Conv2d(3, 16, kernel_size=5),
            #220
            nn.AvgPool2d(2, stride=2),
            #110
            nn.Conv2d(16, 32, kernel_size=3),
            #108
            nn.Tanh(),
            nn.AvgPool2d(2, stride=2),
            #54
            nn.Conv2d(32, 64, kernel_size=5),
            #50
            nn.Tanh())

        self.classifier = nn.Sequential(
            nn.Linear(50 * 50 * 64, 120),
            nn.Tanh(),
            nn.Linear(120, 84),
            nn.Tanh(),
            nn.Linear(84, num_classes))

    def forward(self, x):
        x = self.features(x)
        x = torch.flatten(x, 1)
        logits = self.classifier(x)
        probs = F.softmax(logits, dim=1)
        return logits
