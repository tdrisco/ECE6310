clear
clc
close all

fileID = fopen('Roc_lab3.txt','r');
data = fscanf(fileID,'%f %f',[2,Inf]);
data = data';

xdata = data(:,1);
ydata = data(:,2);

figure('Color','w');
plot(xdata,ydata,'ko','markerfacecolor','k','markersize',1);
hold on
plot(xdata,ydata,'k-');
%axis([-0.3 0.5 0 1.2]);
xlabel("False Positive Rate (FPR)");
ylabel("True Positive Rate (TPR)");
