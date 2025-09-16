#include "opencv2/opencv.hpp"
#include <iostream>
#include <algorithm>

using namespace cv;
using namespace std;

Mat image_preprocessing(Mat);
Mat detect_netdefect(Mat, Mat);

struct meshinfo
{
	int label;
	int x;
	float area;
};
bool cmp(meshinfo mesh1, meshinfo mesh2)
{
	if (mesh1.x < mesh2.x) return true;

	return false;
}

bool cmp_area(meshinfo mesh1, meshinfo mesh2)
{
	if (mesh1.area < mesh2.area) return true;
	else if (mesh1.area == mesh2.area)
	{
		if (mesh1.label < mesh2.label) return true;
	}
	return false;
}


int main()
{
	Mat img, preprocessingimg;
	vector<String> IMGarray;

	String path("image/*.*");
	//cout << "Put the directory path that have images (ex. C:/your/directory/path/*.*)" << endl;
	//getline(cin, path);

	glob(path, IMGarray, false);

	cout << IMGarray.size() << endl;

	if (IMGarray.size() == 0)
		cout << "No image in directory, check the directory or path" << endl;

	else
	{
		for (int cnt = 0; cnt < IMGarray.size(); cnt++)
		{
			img = imread(IMGarray[cnt]);
			if (img.empty()) cout << "No image in directory, check the directory or path" << endl;
			resize(img, img, Size(1000, 500));

			preprocessingimg = image_preprocessing(img);

			Mat resultimg = detect_netdefect(img, preprocessingimg);

			imshow("Result", resultimg);
			waitKey(0);

		}
	}

	return 0;
}

Mat image_preprocessing(Mat img)
{
	Mat GsubR_filter, bgr_channle[3];

	split(img, bgr_channle);

	Mat RsubG = bgr_channle[2] - bgr_channle[1];
	equalizeHist(RsubG, RsubG);
	threshold(RsubG, RsubG, 50, 255, THRESH_BINARY);

	Mat GsubR = bgr_channle[1] - bgr_channle[2];
	equalizeHist(GsubR, GsubR);
	GaussianBlur(GsubR, GsubR_filter, Size(0, 0), 8, 0);

	Mat kernel = getStructuringElement(MORPH_CROSS, Size(11, 11));
	morphologyEx(GsubR_filter, GsubR_filter, MORPH_BLACKHAT, kernel);
	GsubR_filter = GsubR_filter * 255;

	Mat Netimg = RsubG + GsubR_filter;
	Netimg = 255 - Netimg;

	return Netimg;
}

Mat detect_netdefect(Mat originimg, Mat preprocessingimg)
{
	Mat labels, stats, centroids;

	int nlabels = connectedComponentsWithStats(preprocessingimg, labels, stats, centroids);

	vector<Rect> hole_boundingbox;
	for (int i = 0; i < nlabels; i++)
	{
		int x = stats.at<int>(i, CC_STAT_LEFT);
		int y = stats.at<int>(i, CC_STAT_TOP);
		int w = stats.at<int>(i, CC_STAT_WIDTH);
		int h = stats.at<int>(i, CC_STAT_HEIGHT);

		Rect box(x, y, w, h);
		hole_boundingbox.push_back(box);
	}

	meshinfo number_area;
	vector<meshinfo> mesh_col;
	vector<vector<meshinfo>> mesh_row;

	int count = 1;

	for (int i = 1; i < nlabels; i++)
	{
		if (stats.at<int>(i, CC_STAT_AREA) < 100) continue;

		number_area.label = i;
		number_area.area = stats.at<int>(i, CC_STAT_AREA);
		number_area.x = stats.at<int>(i, CC_STAT_LEFT);

		mesh_col.push_back(number_area);

		if (stats.at<int>(i, CC_STAT_TOP) > ((originimg.rows / 3) * count))
		{
			mesh_row.push_back(mesh_col);
			mesh_col.erase(mesh_col.begin(), mesh_col.end());
			count += 1;
		}
	}

	mesh_row.push_back(mesh_col);

	vector<meshinfo> detect_area;
	vector<int> first_detect;
	double avr_area = 0, diff_area = 0;

	for (int i = 0; i < mesh_row.size(); i++)
	{
		sort(mesh_row[i].begin(), mesh_row[i].end(), cmp);

		for (int j = 0; j < mesh_row[i].size(); j++)
		{

			detect_area.push_back(mesh_row[i][j]);
			avr_area += mesh_row[i][j].area;

			if (detect_area.size() % 9 == 0)
			{

				sort(detect_area.begin(), detect_area.end(), cmp_area);

				avr_area = avr_area / 9;
				diff_area = detect_area.back().area - detect_area.at(7).area;

				if (diff_area > avr_area)
				{
					int number = detect_area.back().label;
					first_detect.push_back(number);

				}

				detect_area.erase(detect_area.begin(), detect_area.end());
				avr_area = 0;
				diff_area = 0;
			}
		}

	}

	int temp = 0;
	for (int i = 0; i < first_detect.size(); i++)
	{
		int x = stats.at<int>(first_detect[i], CC_STAT_LEFT);
		int y = stats.at<int>(first_detect[i], CC_STAT_TOP);
		int w = stats.at<int>(first_detect[i], CC_STAT_WIDTH);
		int h = stats.at<int>(first_detect[i], CC_STAT_HEIGHT);
		Rect standard(x, y, w, h);
		for (int j = 0; j < hole_boundingbox.size(); j++)
		{
			Rect occlusionbox = standard & hole_boundingbox[j];
			if (occlusionbox.area() > 0)
			{
				temp += 1;
			}
		}

		if (temp > 9)
		{
			rectangle(originimg, standard, Scalar(0, 0, 255), 2);
		}
	}

	return originimg;
}