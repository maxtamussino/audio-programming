data = csvread("scopedata_filtering.csv");

time = data(:,1) * 1000;
raw = data(:,2);
smooth = data(:,3);
filtered = data(:,4);

figure('Position', [100 100 1100 300]);
plot(time, raw);
hold on
plot(time, smooth);
hold on
plot(time, filtered, 'g');
hold on

legend('Raw input', 'First stage', 'Second stage');
set(gca, 'FontSize', 14);
set(gcf,'color','w');
xlabel('Time [ms]');
ylabel('Acceleration [g]');