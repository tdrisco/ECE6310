% Tim Driscoll 
% Lab 7 - Motion Capture
% 11/9/21

clear
clc
close all

%% Read in the raw data outputed from c file
fileID = fopen('acc_gyro_raw.txt','r');
measurements = fscanf(fileID,'%f %f %f %f %f %f %f',[7,Inf]);
measurements = measurements';
[r,c] = size(measurements);

%% Read in the smoothed data outputed from c file
fileID = fopen('acc_gyro_smoothed.txt','r');
measurements_smoothed = fscanf(fileID,'%f %f %f %f %f %f %f',[7,Inf]);
measurements_smoothed = measurements_smoothed';

t = linspace(1,r,r);
label = {'acc_x','acc_y','acc_z','gyro_x','gyro_y','gyro_z'};

%% Plot all the raw and smoothed data 
for i = 2:1:7
    %raw and smoothed data plotted as side by side subplots
    figure('Color','w');
    subplot(1,2,1)
    plot(t,measurements(:,i),'k-') %plotting the raw data position
    xlabel("Time [T]");
    ylabel(label{i-1});
    title("Raw Data")
    subplot(1,2,2)
    plot(t,measurements_smoothed(:,i),'k-') %plotting the actual position
    xlabel("Time [T]");
    ylabel(label{i-1});
    title("Smoothed Data")
end