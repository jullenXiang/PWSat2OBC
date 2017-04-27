#include "mission.h"
#include "antenna/antenna.h"
#include "logger/logger.h"
#include "obc.h"
#include "system.h"
#include "terminal.h"

extern mission::ObcMission Mission;

void SuspendMission(std::uint16_t argc, char* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    LOG(LOG_LEVEL_INFO, "Received request to suspend automatic mission processing.");
    Mission.Suspend();
}

void ResumeMission(std::uint16_t argc, char* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    LOG(LOG_LEVEL_INFO, "Received request to resume automatic mission processing.");
    Mission.Resume();
}

void RunMission(std::uint16_t argc, char* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    LOG(LOG_LEVEL_INFO, "Received request to run mission processing once");
    Mission.RequestSingleIteration();
}

void SetFiboIterations(std::uint16_t argc, char* argv[])
{
    if (argc != 1)
    {
        Main.terminal.Puts("set_fibo_iterations <iterations>");
        return;
    }

    std::uint16_t iters = atoi(argv[0]);

    Main.Experiments.Fibo.Iterations(iters);
}

void RequestExperiment(std::uint16_t argc, char* argv[])
{
    if (argc != 1)
    {
        Main.terminal.Puts("request_experiment <experimentType>");
        return;
    }

    std::uint16_t expType = atoi(argv[0]);

    Main.Experiments.ExperimentsController.RequestExperiment(gsl::narrow_cast<experiments::ExperimentCode>(expType));
}

void AbortExperiment(std::uint16_t argc, char* argv[])
{
    UNUSED(argc, argv);

    Main.Experiments.ExperimentsController.AbortExperiment();
}

void ExperimentInfo(std::uint16_t argc, char* argv[])
{
    LOG(LOG_LEVEL_DEBUG, "ExpInfo");
    //    Main.terminal.Puts("X");

    UNUSED(argc, argv);
    auto state = Main.Experiments.ExperimentsController.CurrentState();

    if (state.RequestedExperiment.HasValue)
        Main.terminal.Printf("Requested\t%d\n", state.RequestedExperiment.Value);
    else
        Main.terminal.Puts("Requested\tNone\n");

    if (state.CurrentExperiment.HasValue)
        Main.terminal.Printf("Current\t%d\n", state.CurrentExperiment.Value);
    else
        Main.terminal.Puts("Current\tNone\n");

    if (state.LastStartResult.HasValue)
        Main.terminal.Printf("LastStartResult\t%d\n", num(state.LastStartResult.Value));
    else
        Main.terminal.Puts("LastStartResult\tNone\n");

    if (state.LastIterationResult.HasValue)
        Main.terminal.Printf("LastIterationResult\t%d\n", num(state.LastIterationResult.Value));
    else
        Main.terminal.Puts("LastIterationResult\tNone\n");

    Main.terminal.Printf("IterationCounter\t%ld\n", state.IterationCounter);
}
