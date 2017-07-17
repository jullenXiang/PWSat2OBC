#include <adcs/SunPointing.hpp>
#include <Eigen/Dense>
#include <cstring>
#include <fstream>
#include <iostream>
#include <system.h>
#include <unistd.h>
#include <gtest/gtest.h>
#include "adcs/experimental/adcsUtConfig.h"
#include "adcs/experimental/Include/adcs/dataFileTools.hpp"

#define COMP_ACCUR 0.01

using adcs::SunPointing;
using adcs::DipoleVec;
using adcs::MagVec;
using adcs::SunsVec;
using adcs::GyroVec;
using Eigen::Matrix;
using Eigen::Matrix3f;
using Eigen::Vector3f;
using Matrix55f = Matrix<float, 5, 5>;

enum ESPDataIdx
{
    ESP_t = 0,                // 1x1
    ESP_mtmMeas = 1,          // 3x1
    ESP_ssMeas = 4,           // 2x1
    ESP_gyroMeas = 6,         // 3x1
    ESP_mtmFlag = 9,          // 1x1
    ESP_ssFlag = 10,          // 1x1
    ESP_gyroFlag = 11,        // 1x1
    ESP_X_EKF_Prev = 16,      // 5x1
    ESP_p_EKF_Prev = 41,      // 5x5
    ESP_ctrlTorquePrev = 44,  // 3x1
    ESP_EKFconvCountPrev = 45,  // 1x1
    ESP_commDipoleSP = 48,    // 3x1
    ESP_ctrlTorque = 51,      // 3x1
    ESP_X_EKF = 56,           // 5x1
    ESP_P_EKF = 81,           // 5x5
    ESP_innov = 86,           // 5x1
    ESP_innovCov = 111,       // 5x5
    ESP_EKFisInit = 112,      // 1x1
    ESP_EKFisConv = 113,      // 1x1
    ESP_EKFconvCount = 114,   // 1x1
    ESPDataIdx_size
};

// cross-validation of skew function against matlab implementation
TEST(sunpointing, cross_validation_skew)
{
    const uint recordLen = 12;   //in: 3x1 + out: 3x3
    Vector3f in;
    Matrix3f res;
    std::ifstream file(
    ADCS_UT_DATA_FILE_PATH "/sunpointing_crossvalidation_skew.csv");
    if (!file)
    {
        FAIL()<< "Cannot find data  file!" << std::endl;
    }
    std::vector<float> record;

    while (!file.eof())
    //for(int i = 0; i< 2; i++)
    {
        record = dataFileTools::getRecord(file);
        if (record.size() != recordLen)
        {
            if (file.eof())
            {
                break;
            }
            else
            {
                FAIL()<< "Data record has size different than expected (got: " << record.size() << " but expected: " << recordLen << ")" << std::endl;
            }
        }

        in << record[0], record[1], record[2];
        res = skew(in);

        //EXPECT_NEAR(res[0], (Matrix3f << record[3],record[4],record[5],record[6],record[7],record[8],record[9],record[10],record[11]).finished(), COMP_ACCUR);
        for (int j = 0; j < res.cols(); j++)
        {
            for (int i = 0; i < res.rows(); i++)
            {
                EXPECT_NEAR(res(i, j), record[3 + i + j * res.cols()],
                        COMP_ACCUR);
#ifdef ADCS_SUNPOINTING_DEBUG
                std::cout << "EXPECT_NEAR(" << res(i, j) << " == " << record[3 + i + j * res.cols()] << ")" << std::endl;
#endif
            }
        }
    }
    file.close();
}

// cross-validation of skew function against matlab implementation
TEST(sunpointing, cross_validation_vecNorm)
{
    const uint recordLen = 4;   //in: 3x1 + out: 1x1
    Vector3f in;
    float res;
    std::ifstream file(
    ADCS_UT_DATA_FILE_PATH "/sunpointing_crossvalidation_vectorNorm.csv");
    if (!file)
    {
        FAIL()<< "Cannot find data  file!" << std::endl;
    }
    std::vector<float> record;

    while (!file.eof())
    //for(int i = 0; i< 2; i++)
    {
        record = dataFileTools::getRecord(file);
        if (record.size() != recordLen)
        {
            if (file.eof())
            {
                break;
            }
            else
            {
                FAIL()<< "Data record has size different than expected (got: " << record.size() << " but expected: " << recordLen << ")" << std::endl;
            }
        }

        in << record[0], record[1], record[2];
        res = in.norm();

        EXPECT_NEAR(res, record[3], COMP_ACCUR);
#ifdef ADCS_SUNPOINTING_DEBUG
        std::cout << "EXPECT_NEAR(" << res << " == " << record[3] << ")" << std::endl;
#endif
    }
    file.close();
}

// cross-validation of matInv function against matlab implementation
TEST(sunpointing, cross_validation_matInv)
{
    const uint recordLen = 50;   //in: 5x5 + out: 5x5
    Matrix55f in;
    Matrix55f res;
    std::ifstream file(
    ADCS_UT_DATA_FILE_PATH "/sunpointing_crossvalidation_matrixInv.csv");
    if (!file)
    {
        FAIL()<< "Cannot find data  file!" << std::endl;
    }
    std::vector<float> record;

    while (!file.eof())
    //for(int i = 0; i< 2; i++)
    {
        record = dataFileTools::getRecord(file);
        if (record.size() != recordLen)
        {
            if (file.eof())
            {
                break;
            }
            else
            {
                FAIL()<< "Data record has size different than expected (got: " << record.size() << " but expected: " << recordLen << ")" << std::endl;
            }
        }

        for (int j = 0; j < in.cols(); j++)
        {
            for (int i = 0; i < in.rows(); i++)
            {
                in(i, j) = record[i + j * in.cols()];
            }
        }
        //res = matInv(in);// accuracy problems
        res = in.inverse();   // perfect result

        //EXPECT_NEAR(res[0], (Matrix3f << record[3],record[4],record[5],record[6],record[7],record[8],record[9],record[10],record[11]).finished(), COMP_ACCUR);
        for (int j = 0; j < res.cols(); j++)
        {
            for (int i = 0; i < res.rows(); i++)
            {
                EXPECT_NEAR(res(i, j), record[25 + i + j * res.cols()],
                        COMP_ACCUR);
#ifdef ADCS_SUNPOINTING_DEBUG
                std::cout << "EXPECT_NEAR(" << res(i, j) << " == " << record[25 + i + j * res.cols()] << ")" << std::endl;
#endif
            }
        }
    }
    file.close();
}

// cross-validation of spinControler function against matlab implementation
TEST(sunpointing, cross_validation_spinCtrl)
{
    const uint recordLen = 12; // 4x(3x1)
    Vector3f inAngRateEst;
    Vector3f inBodyEst;
    Vector3f inMtmMeas;
    Vector3f outDipole;
    SunPointing::State state; //just parameterisation
    std::ifstream file(
    ADCS_UT_DATA_FILE_PATH "/sunpointing_crossvalidation_SpinController.csv");
    if (!file)
    {
        FAIL()<< "Cannot find data  file!" << std::endl;
    }
    std::vector<float> record;

    while (!file.eof())
    //for(int i = 0; i< 2; i++)
    {
        record = dataFileTools::getRecord(file);
        if (record.size() != recordLen)
        {
            if (file.eof())
            {
                break;
            }
            else
            {
                FAIL()<< "Data record has size different than expected (got: " << record.size() << " but expected: " << recordLen << ")" << std::endl;
            }
        }

        for (int j = 0; j < inAngRateEst.cols(); j++)
        {
            for (int i = 0; i < inAngRateEst.rows(); i++) //TODO do sth about indexing
            {
                inAngRateEst(i, j) = record[0 + i + j * inAngRateEst.cols()];
                inBodyEst(i, j) = record[3 + i + j * inAngRateEst.cols()];
                inMtmMeas(i, j) = record[6 + i + j * inAngRateEst.cols()];
            }
        }

        SpinController(outDipole, inAngRateEst, inBodyEst, inMtmMeas, state);

        //EXPECT_NEAR(res[0], (Matrix3f << record[3],record[4],record[5],record[6],record[7],record[8],record[9],record[10],record[11]).finished(), COMP_ACCUR);
        for (int j = 0; j < outDipole.cols(); j++)
        {
            for (int i = 0; i < outDipole.rows(); i++)
            {
                EXPECT_NEAR(outDipole(i, j),
                        record[9 + i + j * outDipole.cols()], COMP_ACCUR);
#ifdef ADCS_SUNPOINTING_DEBUG
                std::cout << "EXPECT_NEAR(" << outDipole(i, j) << " == " << record[9 + i + j * outDipole.cols()] << ")" << std::endl;
#endif
            }
        }
    }
    file.close();
}

// cross-validation of EKFInitialisation function against matlab implementation
TEST(sunpointing, cross_validation_ekfInit)
{
    uint record_cntr = 0;
    const uint recordLen = 5 + 25 + 5 + 25 + 2 + 3;
    //inputs
    Vector2f ssMeas;
    Vector3f gyrMeas;
    //outputs
    Vector5f xEkf;
    Matrix55f pEkf;
    Vector5f innov;
    Matrix55f innovCov;

    SunPointing::State state; //just parameterisation

    std::ifstream file(
    ADCS_UT_DATA_FILE_PATH "/sunpointing_crossvalidation_init.csv");
    if (!file)
    {
        FAIL()<< "Cannot find data  file!" << std::endl;
    }
    std::vector<float> record;

    while (!file.eof())
    //for(int i = 0; i< 2; i++)
    {
        record_cntr = 0;
        record = dataFileTools::getRecord(file);
        if (record.size() != recordLen)
        {
            if (file.eof())
            {
                break;
            }
            else
            {
                FAIL()<< "Data record has size different than expected (got: " << record.size() << " but expected: " << recordLen << ")" << std::endl;
            }
        }

        ssMeas[0] = record[record_cntr++]; //TODO put this to function!!!
        ssMeas[1] = record[record_cntr++];
        gyrMeas[0] = record[record_cntr++];
        gyrMeas[1] = record[record_cntr++];
        gyrMeas[2] = record[record_cntr++];

        EKFinitialization(xEkf, pEkf, innov, innovCov, ssMeas, gyrMeas, state);

        //EXPECT_NEAR(res[0], (Matrix3f << record[3],record[4],record[5],record[6],record[7],record[8],record[9],record[10],record[11]).finished(), COMP_ACCUR);
        for (int j = 0; j < xEkf.cols(); j++) //TODO put this test to function!!!
        {
            for (int i = 0; i < xEkf.rows(); i++)
            {
                EXPECT_NEAR(xEkf(i, j), record[record_cntr], COMP_ACCUR);
#ifdef ADCS_SUNPOINTING_DEBUG
                std::cout << "EXPECT_NEAR(" << xEkf(i, j) << " == "
                << record[record_cntr] << ")" << std::endl;
#endif
                record_cntr++;
            }
        }

        for (int j = 0; j < pEkf.cols(); j++)
        {
            for (int i = 0; i < pEkf.rows(); i++)
            {
                EXPECT_NEAR(pEkf(i, j), record[record_cntr], COMP_ACCUR);
#ifdef ADCS_SUNPOINTING_DEBUG
                std::cout << "EXPECT_NEAR(" << pEkf(i, j) << " == " << record[record_cntr] << ")" << std::endl;
#endif
                record_cntr++;
            }
        }

        for (int j = 0; j < innov.cols(); j++)
        {
            for (int i = 0; i < innov.rows(); i++)
            {
                EXPECT_NEAR(innov(i, j), record[record_cntr], COMP_ACCUR);
#ifdef ADCS_SUNPOINTING_DEBUG
                std::cout << "EXPECT_NEAR(" << innov(i, j) << " == "
                << record[record_cntr] << ")" << std::endl;
#endif
                record_cntr++;
            }
        }

        for (int j = 0; j < innovCov.cols(); j++)
        {
            for (int i = 0; i < innovCov.rows(); i++)
            {
                EXPECT_NEAR(innovCov(i, j), record[record_cntr], COMP_ACCUR);
#ifdef ADCS_SUNPOINTING_DEBUG
                std::cout << "EXPECT_NEAR(" << innovCov(i, j) << " == " << record[record_cntr] << ")" << std::endl;
#endif
                record_cntr++;
            }
        }
    }
    file.close();
}

// cross-validation of sunpointing against matlab implementation
TEST(sunpointing, cross_validation)
{
    std::cout << "SUNPOINTING TEST... NOT REPRESENTATIVE!!!" << std::endl;

    std::ifstream file(
    ADCS_UT_DATA_FILE_PATH "/sunpointing_crossvalidation.csv");
    if (!file)
    {
        FAIL()<< "Cannot find data  file!" << std::endl;
    }
    std::vector<float> record;

    SunPointing::State state; // should be initialised by first measurement
    SunPointing::Parameters params;

    SunPointing sp;
    sp.initialize(state, params);

    // matlab sim is working with different units
    // input: Sim [T] --> OBC [1e-7 T]
    float input_scale = 1.0f;    //1e7f;
    // output: Sim [Am2] --> OBC [1e-4 Am2]
    float output_scale = 1e4f;

    //while (!file.eof())
    for (int i = 0; i < 2; i++)
    {
        record = dataFileTools::getRecord(file); // TODO change to sunpointing data pool
        if (record.size() != ESPDataIdx_size)
        {
            if (file.eof())
            {
                break;
            }
            else
            {
                FAIL()<< "Data record has size different than expected (got: " << record.size() << " but expected: " << ESPDataIdx_size << ")" << std::endl;
            }
        }
//#define ADCS_SUNPOINTING_DEBUG
#ifdef ADCS_SUNPOINTING_DEBUG
        std::cout << "time: " << record[ESP_t] << std::endl;
        std::cout << "in0: " << record[ESP_mtmMeas] << " " << input_scale << " " << record[ESP_mtmMeas] * input_scale << " " << (int)(record[1] * input_scale)
        << std::endl;
        std::cout << "in1: " << record[ESP_ssMeas] << " " << input_scale << " " << record[ESP_ssMeas] * input_scale << " " << (int)(record[2] * input_scale)
        << std::endl;
        std::cout << "in2: " << record[ESP_gyroMeas] << " " << input_scale << " " << record[ESP_gyroMeas] * input_scale << " " << (int)(record[3] * input_scale)
        << std::endl;
        std::cout << "out0: " << record[ESP_commDipoleSP] << " " << output_scale << " " << record[ESP_commDipoleSP] * output_scale << " "
        << (int)(record[ESP_commDipoleSP] * output_scale) << std::endl;
        std::cout << "out1: " << record[ESP_commDipoleSP+1] << " " << output_scale << " " << record[ESP_commDipoleSP+1] * output_scale << " "
        << (int)(record[ESP_commDipoleSP+1] * output_scale) << std::endl;
        std::cout << "out2: " << record[ESP_commDipoleSP+2] << " " << output_scale << " " << record[ESP_commDipoleSP+2] * output_scale << " "
        << (int)(record[ESP_commDipoleSP+2] * output_scale) << std::endl;
#else
        UNUSED1(input_scale);
#endif
        DipoleVec dipole =
        { 0, 0, 0 };
        MagVec mtmMeas =
        { int32_t(record[ESP_mtmMeas + 0]), int32_t(record[ESP_mtmMeas + 1]),
                int32_t(record[ESP_mtmMeas + 2]) };
        bool mtmFlag = (record[ESP_mtmFlag] != 0);
        SunsVec ssMeas =
        { record[ESP_ssMeas + 0], record[ESP_ssMeas + 1] };
        bool ssFlag = (record[ESP_ssFlag] != 0);
        GyroVec gyrMeas =
        { record[ESP_gyroMeas + 0], record[ESP_gyroMeas + 1],
                record[ESP_gyroMeas + 2] };
        bool gyrFlag = (record[ESP_gyroFlag] != 0);
        sp.step(dipole, mtmMeas, mtmFlag, ssMeas, ssFlag, gyrMeas, gyrFlag,
                state);

        EXPECT_NEAR(dipole[0], record[ESP_commDipoleSP + 0] * output_scale,
                COMP_ACCUR);
        EXPECT_NEAR(dipole[1], record[ESP_commDipoleSP + 1] * output_scale,
                COMP_ACCUR);
        EXPECT_NEAR(dipole[2], record[ESP_commDipoleSP + 2] * output_scale,
                COMP_ACCUR);

#ifdef ADCS_SUNPOINTING_DEBUG
        std::cout << "EXPECT_NEAR(" << dipole[0] << " == " << record[ESP_commDipoleSP+0] * output_scale << ")" << std::endl;
        std::cout << "EXPECT_NEAR(" << dipole[1] << " == " << record[ESP_commDipoleSP+1] * output_scale << ")" << std::endl;
        std::cout << "EXPECT_NEAR(" << dipole[2] << " == " << record[ESP_commDipoleSP+2] * output_scale << ")" << std::endl;
#endif
    }
    file.close();
}
